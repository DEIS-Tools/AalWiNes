//
// Created by Morten on 18-03-2020.
//

#define BOOST_TEST_MODULE SyntacticNetwork

#include <boost/test/unit_test.hpp>
#include <aalwines/model/Network.h>
#include <aalwines/query/QueryBuilder.h>
#include <aalwines/Verifier.h>
#include <fstream>
#include <aalwines/synthesis/RouteConstruction.h>
using namespace aalwines;

Network construct_synthetic_network(size_t nesting = 1){
    size_t router_size = 5 * nesting;
    std::string router_name = "Router";
    std::vector<std::string> router_names;
    std::vector<std::unique_ptr<Router> > _routers;
    std::vector<const Interface*> _all_interfaces;
    Network::routermap_t _mapping;

    for(size_t i = 0; i < router_size; i++) {
        router_names.push_back(router_name + std::to_string(i));
    }

    size_t network_node = 0;
    bool nested = nesting > 1;
    bool fall_through = false;
    size_t last_nesting_begin = nesting * 5 - 5;

    std::vector<std::vector<std::string>> links;

    for(size_t i = 0; i < router_size; i++, network_node++) {
        router_name = router_names[i];
        _routers.emplace_back(std::make_unique<Router>(i));
        Router &router = *_routers.back().get();
        router.add_name(router_name);
        router.get_interface("i" + router_names[i], _all_interfaces);
        auto res = _mapping.insert(router_name);
        _mapping.get_data(res.second) = &router;
        switch (network_node) {
            case 1:
                router.get_interface(router_names[i - 1], _all_interfaces);
                router.get_interface(router_names[i + 2], _all_interfaces);
                links.push_back({router_names[i - 1], router_names[i + 2]} );
                break;
            case 2:
                router.get_interface(router_names[i + 1], _all_interfaces);
                router.get_interface(router_names[i + 2], _all_interfaces);
                links.push_back({router_names[i + 1], router_names[i + 2]});
                if(nested){
                    router.get_interface(router_names[i + 6], _all_interfaces);
                    links.back().push_back({router_names[i + 6]});
                } else {
                    router.get_interface(router_names[i - 2], _all_interfaces);
                    links.back().push_back({router_names[i - 2]});
                }
                break;
            case 3:
                router.get_interface(router_names[i - 2], _all_interfaces);
                router.get_interface(router_names[i + 1], _all_interfaces);
                router.get_interface(router_names[i - 1], _all_interfaces);
                links.push_back({router_names[i - 2], router_names[i + 1], router_names[i - 1]});
                if(i != 3) {
                    router.get_interface(router_names[i - 6], _all_interfaces);
                    links.back().push_back({router_names[i - 6]});
                }
                break;
            case 4:
                router.get_interface(router_names[i - 2], _all_interfaces);
                router.get_interface(router_names[i - 1], _all_interfaces);
                links.push_back({router_names[i - 2], router_names[i - 1]});
                break;
            case 5:
                network_node = 0;   //Fall through
                if(i == last_nesting_begin){
                    nested = false;
                }
                fall_through = true;
            case 0:
                router.get_interface(router_names[i + 1], _all_interfaces);
                links.push_back({router_names[i + 1]});
                if(nested){
                    router.get_interface(router_names[i + 5], _all_interfaces);
                    links.back().push_back({router_names[i + 5]});
                } else {
                    router.get_interface(router_names[i + 2], _all_interfaces);
                    links.back().push_back({router_names[i + 2]});
                }
                if(fall_through){
                    router.get_interface(router_names[i - 5], _all_interfaces);
                    links.back().push_back({router_names[i - 5]});
                    fall_through = false;
                }
                break;
            default:
                throw base_error("Something went wrong in the construction");
        }
    }
    for (size_t i = 0; i < router_size; ++i) {
        for (const auto &other : links[i]) {
            auto res1 = _mapping.exists(router_names[i]);
            assert(res1.first);
            auto res2 = _mapping.exists(other);
            if(!res2.first) continue;
            _mapping.get_data(res1.second)->find_interface(other)->make_pairing(_mapping.get_data(res2.second)->find_interface(router_names[i]));
        }
    }
    Network network(std::move(_mapping), std::move(_routers), std::move(_all_interfaces));
    network.add_null_router(); //Last router
    return network;
}

void performance_query(const std::string& query, Network& synthetic_network, Builder builder, std::ostream& trace_stream){
    //Adapt to existing query parser
    std::istringstream qstream(query);
    builder.do_parse(qstream);

    auto& q = builder._result[0];
    std::vector<Query::mode_t> modes{q.approximation()};
    std::pair<size_t, size_t> reduction;
    q.set_approximation(modes[0]);
    q.compile_nfas();
    NetworkPDAFactory factory(q, synthetic_network, builder.all_labels());
    auto problem_instance = factory.compile(q.construction(), q.destruction());
    //reduction = pdaaal::Reducer::reduce(pda, 0, pda.initial(), pda.terminal());

    std::stringstream results;
    stopwatch verification_time_post(false);

    trace_stream << std::endl << "Post*: " <<std::endl;

    verification_time_post.start();
    auto solver_result1 = pdaaal::Solver::post_star_accepts<pdaaal::Trace_Type::Any>(*problem_instance);
    BOOST_CHECK(solver_result1);
    auto pda_trace = pdaaal::Solver::get_trace<pdaaal::Trace_Type::Any>(*problem_instance);
    verification_time_post.stop();
    auto json_trace = factory.get_json_trace(pda_trace);
    trace_stream << json_trace.dump();

    results << std::endl << "post*-time: " << verification_time_post.duration() << std::endl;

    BOOST_TEST_MESSAGE(results.str());
}

void build_query(const std::string& query, Network& synthetic_network, Builder builder){
    //Adapt to existing query parser
    std::istringstream qstream(query);
    builder.do_parse(qstream);

    Verifier verifier;
    for(auto& q : builder._result) {
        auto output = verifier.run_once(builder, q);
        auto result = output["result"].get<utils::outcome_t>();
        BOOST_CHECK_EQUAL(result, utils::outcome_t::YES);
        std::cout << output << std::endl;
    }
}

BOOST_AUTO_TEST_CASE(NetworkInjectionAndTrace) {
    Network synthetic_network = construct_synthetic_network(2);

    std::vector<const Router*> path {synthetic_network.get_router(0),
                                     synthetic_network.get_router(5),
                                     synthetic_network.get_router(7),
                                     synthetic_network.get_router(9)};
    uint64_t i = 42;
    auto next_label = [&i](){return i++;};
    auto success = RouteConstruction::make_data_flow(
            synthetic_network.get_router(0)->find_interface("iRouter0"),
            synthetic_network.get_router(9)->find_interface("iRouter9"),
            next_label, path);

    Network synthetic_network2 = construct_synthetic_network();
    path = {synthetic_network2.get_router(0),
            synthetic_network2.get_router(2),
            synthetic_network2.get_router(3)};
    auto success1 = RouteConstruction::make_data_flow(
            synthetic_network2.get_router(0)->find_interface("iRouter0"),
            synthetic_network2.get_router(3)->find_interface("iRouter3"),
            next_label, path);

    BOOST_CHECK_EQUAL(success, success1);

    synthetic_network.inject_network(
            synthetic_network.get_router(5)->find_interface("Router7"),
            std::move(synthetic_network2),
            synthetic_network2.get_router(0)->find_interface("iRouter0"),
            synthetic_network2.get_router(3)->find_interface("iRouter3"),
            47,
            50);

    BOOST_TEST_MESSAGE("After: ");
    std::stringstream s_after;
    synthetic_network.print_simple(s_after);
    BOOST_TEST_MESSAGE(s_after.str());

    synthetic_network.prepare_tables(); synthetic_network.pre_process();

    Builder builder(synthetic_network);
    {
        std::string query("<.*> [.#Router0] .* ['Router2\\''#.] <.*> 0 OVER \n"
                          "<.*> [.#Router0] .* [Router5#.] <.*> 0 OVER \n"
                          "<.*> [.#Router0] .* [Router7#.] <.*> 0 OVER \n"
                          "<.*> [.#Router0] .* [Router9#.] <.*> 0 OVER \n"
                          "<.*> [.#Router0] .* ['Router0\\''#.] <.*> 0 OVER \n"
        );
        build_query(query, synthetic_network, builder);
    }
}

BOOST_AUTO_TEST_CASE(NetworkInjectionAndTrace1) {
    Network synthetic_network = construct_synthetic_network(1);

    std::vector<const Router*> path {synthetic_network.get_router(0),
                                     synthetic_network.get_router(2),
                                     synthetic_network.get_router(4)};
    uint64_t i = 42;
    auto next_label = [&i](){return i++;};
    auto success = RouteConstruction::make_data_flow(
            synthetic_network.get_router(0)->find_interface("iRouter0"),
            synthetic_network.get_router(4)->find_interface("iRouter4"),
            next_label, path);

    Network synthetic_network2 = construct_synthetic_network();

    path = {synthetic_network2.get_router(0),
            synthetic_network2.get_router(2),
            synthetic_network2.get_router(3)};
    auto success1 = RouteConstruction::make_data_flow(
            synthetic_network2.get_router(0)->find_interface("iRouter0"),
            synthetic_network2.get_router(3)->find_interface("iRouter3"),
            next_label, path);

    BOOST_CHECK_EQUAL(success, success1);

    std::stringstream s_before;
    synthetic_network2.print_simple(s_before);
    BOOST_TEST_MESSAGE(s_before.str());

    synthetic_network.inject_network(
            synthetic_network.get_router(0)->find_interface("Router2"),
            std::move(synthetic_network2),
            synthetic_network2.get_router(0)->find_interface("iRouter0"),
            synthetic_network2.get_router(3)->find_interface("iRouter3"),
            46,
            49);

    BOOST_TEST_MESSAGE("After: ");
    std::stringstream s_after;
    synthetic_network.print_simple(s_after);
    BOOST_TEST_MESSAGE(s_after.str());

    synthetic_network.prepare_tables(); synthetic_network.pre_process();

    Builder builder(synthetic_network);
    {
        std::string query("<.*> [.#Router0] .* ['Router2\\''#.] <.*> 0 OVER \n"
                          "<.*> [.#Router0] .* [Router2#.] <.*> 0 OVER \n"
                          "<.*> [.#Router0] .* ['Router0\\''#.] <.*> 0 OVER \n"
        );
        build_query(query, synthetic_network, builder);
    }
}

BOOST_AUTO_TEST_CASE(NetworkInjectionAndTrace2) {
    Network synthetic_network = construct_synthetic_network(5);

    std::vector<const Router*> path {synthetic_network.get_router(0),
                                     synthetic_network.get_router(5),
                                     synthetic_network.get_router(10),
                                     synthetic_network.get_router(15),
                                     synthetic_network.get_router(20),
                                     synthetic_network.get_router(22),
                                     synthetic_network.get_router(24)};
    uint64_t i = 42;
    auto next_label = [&i](){return i++;};
    auto success = RouteConstruction::make_data_flow(
            synthetic_network.get_router(0)->find_interface("iRouter0"),
            synthetic_network.get_router(24)->find_interface("iRouter24"),
            next_label, path);

    Network synthetic_network2 = construct_synthetic_network(2);

    path = {synthetic_network2.get_router(0),
            synthetic_network2.get_router(5),
            synthetic_network2.get_router(6),
            synthetic_network2.get_router(8),
            synthetic_network2.get_router(2),
            synthetic_network2.get_router(3)};
    auto success1 = RouteConstruction::make_data_flow(
            synthetic_network2.get_router(0)->find_interface("iRouter0"),
            synthetic_network2.get_router(3)->find_interface("iRouter3"),
            next_label, path);

    BOOST_CHECK_EQUAL(success, success1);

    synthetic_network.inject_network(
            synthetic_network.get_router(20)->find_interface("Router22"),
            std::move(synthetic_network2),
            synthetic_network2.get_router(0)->find_interface("iRouter0"),
            synthetic_network2.get_router(3)->find_interface("iRouter3"),
            50,
            56);

    BOOST_TEST_MESSAGE("After: ");
    std::stringstream s_after;
    synthetic_network.print_simple(s_after);
    BOOST_TEST_MESSAGE(s_after.str());

    synthetic_network.prepare_tables(); synthetic_network.pre_process();

    Builder builder(synthetic_network);
    {
        std::string query("<.*> [.#Router0] .* [Router24#.] <.*> 0 OVER \n"
                          "<.*> [.#Router0] .* ['Router2\\''#.] <.*> 0 OVER \n"
                          "<.*> [.#Router0] .* ['Router0\\''#.] <.*> 0 OVER \n"
        );
        build_query(query, synthetic_network, builder);
    }
}

BOOST_AUTO_TEST_CASE(NetworkConcatenationAndTrace) {
    Network synthetic_network = construct_synthetic_network(1);

    std::vector<const Router*> path {synthetic_network.get_router(0),
                                     synthetic_network.get_router(1),
                                     synthetic_network.get_router(3),
                                     synthetic_network.get_router(2),
                                     synthetic_network.get_router(4),
                                     synthetic_network.get_router(3)};
    uint64_t i = 42;
    auto next_label = [&i](){return i++;};
    auto success = RouteConstruction::make_data_flow(
            synthetic_network.get_router(0)->find_interface("iRouter0"),
            synthetic_network.get_router(3)->find_interface("iRouter3"),
            next_label, path);

    Network synthetic_network2 = construct_synthetic_network(2);

    path = {synthetic_network2.get_router(0),
            synthetic_network2.get_router(5),
            synthetic_network2.get_router(6),
            synthetic_network2.get_router(8),
            synthetic_network2.get_router(2),
            synthetic_network2.get_router(3)};
    i--;
    auto success1 = RouteConstruction::make_data_flow(
            synthetic_network2.get_router(0)->find_interface("iRouter0"),
            synthetic_network2.get_router(3)->find_interface("iRouter3"),
            next_label, path);

    BOOST_CHECK_EQUAL(success, success1);

    synthetic_network.concat_network(
            synthetic_network.get_router(3)->find_interface("iRouter3"), std::move(synthetic_network2),
            synthetic_network2.get_router(0)->find_interface("iRouter0"), 48);

    BOOST_TEST_MESSAGE("After: ");
    std::stringstream s_after;
    synthetic_network.print_simple(s_after);
    BOOST_TEST_MESSAGE(s_after.str());

    synthetic_network.prepare_tables(); synthetic_network.pre_process();

    Builder builder(synthetic_network);
    {
        std::string query("<.*> [.#Router0] .* ['Router3\\''#.] <.*> 0 OVER \n"
                          "<.*> [.#Router0] .* ['Router2\\''#.] <.*> 0 OVER \n"
                          "<.*> [.#Router0] .* ['Router0\\''#.] <.*> 0 OVER \n"
        );
        build_query(query, synthetic_network, builder);
    }
}

BOOST_AUTO_TEST_CASE(NetworkConcatenationAndTrace1) {
    Network synthetic_network = construct_synthetic_network(5);

    std::vector<const Router*> path {synthetic_network.get_router(0),
                                     synthetic_network.get_router(5),
                                     synthetic_network.get_router(10),
                                     synthetic_network.get_router(15),
                                     synthetic_network.get_router(20),
                                     synthetic_network.get_router(22),
                                     synthetic_network.get_router(24)};
    uint64_t i = 42;
    auto next_label = [&i](){return i++;};
    auto success = RouteConstruction::make_data_flow(
            synthetic_network.get_router(0)->find_interface("iRouter0"),
            synthetic_network.get_router(24)->find_interface("iRouter24"),
            next_label, path);

    Network synthetic_network2 = construct_synthetic_network(2);

    path = {synthetic_network2.get_router(0),
            synthetic_network2.get_router(5),
            synthetic_network2.get_router(6),
            synthetic_network2.get_router(8),
            synthetic_network2.get_router(2),
            synthetic_network2.get_router(3)};
    i--;
    auto success1 = RouteConstruction::make_data_flow(
            synthetic_network2.get_router(0)->find_interface("iRouter0"),
            synthetic_network2.get_router(3)->find_interface("iRouter3"),
            next_label, path);

    BOOST_CHECK_EQUAL(success, success1);

    synthetic_network.concat_network(
            synthetic_network.get_router(24)->find_interface("iRouter24"), std::move(synthetic_network2),
            synthetic_network2.get_router(0)->find_interface("iRouter0"), 48);

    synthetic_network.prepare_tables(); synthetic_network.pre_process();

    Builder builder(synthetic_network);
    {
        std::string query("<.*> [.#Router0] .* ['Router3\\''#.] <.*> 0 OVER \n"
                          "<.*> [.#Router0] .* ['Router2\\''#.] <.*> 0 OVER \n"
                          "<.*> [.#Router0] .* ['Router0\\''#.] <.*> 0 OVER \n"
        );
        build_query(query, synthetic_network, builder);
    }
}

BOOST_AUTO_TEST_CASE(SyntheticNetworkPerformance) {
    Network synthetic_network = construct_synthetic_network(1);

    std::vector<const Router*> path {synthetic_network.get_router(0),
                                     synthetic_network.get_router(2),
                                     synthetic_network.get_router(4)};
    uint64_t i = 42;
    auto next_label = [&i](){return i++;};
    auto success = RouteConstruction::make_data_flow(
            synthetic_network.get_router(0)->find_interface("iRouter0"),
            synthetic_network.get_router(4)->find_interface("iRouter4"),
            next_label, path);

    BOOST_CHECK_EQUAL(success, true);

    synthetic_network.prepare_tables(); synthetic_network.pre_process();

    std::stringstream trace;
    Builder builder(synthetic_network);
    {
        std::string query("<.*> [.#Router0] .* [Router4#.] <.*> 0 OVER \n");
        performance_query(query, synthetic_network, builder, trace);
    }
}

BOOST_AUTO_TEST_CASE(SyntheticNetworkPerformance1) {
    Network synthetic_network = construct_synthetic_network(5);

    std::vector<const Router*> path {synthetic_network.get_router(0),
                                     synthetic_network.get_router(5),
                                     synthetic_network.get_router(10),
                                     synthetic_network.get_router(15),
                                     synthetic_network.get_router(20),
                                     synthetic_network.get_router(22),
                                     synthetic_network.get_router(24)};
    uint64_t i = 42;
    auto next_label = [&i](){return i++;};
    auto success = RouteConstruction::make_data_flow(
            synthetic_network.get_router(0)->find_interface("iRouter0"),
            synthetic_network.get_router(24)->find_interface("iRouter24"),
            next_label, path);

    BOOST_CHECK_EQUAL(success, true);

    synthetic_network.prepare_tables(); synthetic_network.pre_process();

    std::stringstream trace;
    Builder builder(synthetic_network);
    {
        std::string query("<.*> [.#Router0] .* [Router24#.] <.*> 0 OVER \n");
        performance_query(query, synthetic_network, builder, trace);
    }
}

BOOST_AUTO_TEST_CASE(SyntheticNetworkPerformanceInjection) {
    Network synthetic_network = construct_synthetic_network(5);

    std::vector<const Router*> path {synthetic_network.get_router(0),
                                     synthetic_network.get_router(5),
                                     synthetic_network.get_router(10),
                                     synthetic_network.get_router(15),
                                     synthetic_network.get_router(20),
                                     synthetic_network.get_router(22),
                                     synthetic_network.get_router(24)};
    uint64_t i = 42;
    auto next_label = [&i](){return i++;};
    auto success = RouteConstruction::make_data_flow(
            synthetic_network.get_router(0)->find_interface("iRouter0"),
            synthetic_network.get_router(24)->find_interface("iRouter24"),
            next_label, path);

    Network synthetic_network2 = construct_synthetic_network(2);

    path = {synthetic_network2.get_router(0),
            synthetic_network2.get_router(5),
            synthetic_network2.get_router(6),
            synthetic_network2.get_router(8),
            synthetic_network2.get_router(2),
            synthetic_network2.get_router(3)};
    auto success1 = RouteConstruction::make_data_flow(
            synthetic_network2.get_router(0)->find_interface("iRouter0"),
            synthetic_network2.get_router(3)->find_interface("iRouter3"),
            next_label, path);

    BOOST_CHECK_EQUAL(success, success1);

    synthetic_network.inject_network(
            synthetic_network.get_router(20)->find_interface("Router22"),
            std::move(synthetic_network2),
            synthetic_network2.get_router(0)->find_interface("iRouter0"),
            synthetic_network2.get_router(3)->find_interface("iRouter3"),
            50,
            56);

    synthetic_network.prepare_tables(); synthetic_network.pre_process();

    std::stringstream trace;
    Builder builder(synthetic_network);
    {
        std::string query("<.*> [.#Router0] .* [Router24#.] <.*> 0 OVER \n");
        performance_query(query, synthetic_network, builder, trace);
    }
    BOOST_TEST_MESSAGE(trace.str());
}

BOOST_AUTO_TEST_CASE(FastRerouteWithQueryTest) {
    Network network = construct_synthetic_network(1);

    uint64_t i = 42;
    auto next_label = [&i](){return i++;};

    std::vector<const Router*> path {network.get_router(0),
                                     network.get_router(1),
                                     network.get_router(3),
                                     network.get_router(4)};

    auto success = RouteConstruction::make_data_flow(
            network.get_router(0)->find_interface("iRouter0"),
            network.get_router(4)->find_interface("iRouter4"),
            [&i](){return i++;}, path);
    BOOST_CHECK_EQUAL(success, true);

    auto fail_interface = network.get_router(3)->find_interface(network.get_router(4)->name());
    success = RouteConstruction::make_reroute(fail_interface, next_label);
    BOOST_CHECK_EQUAL(success, true);

    network.prepare_tables(); network.pre_process();

    Builder builder(network);
    {
        std::string query("<.*> [.#Router0] .* [Router4#.] <.*> 1 OVER \n"
        );
        build_query(query, network, builder);
    }
}