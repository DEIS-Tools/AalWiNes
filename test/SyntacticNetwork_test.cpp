//
// Created by Morten on 18-03-2020.
//

#define BOOST_TEST_MODULE SyntacticNetwork

#include <boost/test/unit_test.hpp>
#include <aalwines/model/Network.h>
#include <aalwines/query/QueryBuilder.h>
#include <pdaaal/Solver_Adapter.h>
#include <fstream>
#include <aalwines/utils/outcome.h>
#include <aalwines/model/NetworkPDAFactory.h>
#include <pdaaal/Reducer.h>

using namespace aalwines;

void add_entry(Interface& from_interface, Interface& to_interface, RoutingTable::op_t type, int weight, int interface_label_in, int interface_label_out = 0) {
    RoutingTable table;
    auto& entry = table.push_entry();
    entry._ingoing = &from_interface;           //From interface
    Query::type_t q_type = Query::MPLS;
    entry._top_label.set_value(q_type, interface_label_in, 0);

    entry._rules.emplace_back();
    entry._rules.back()._via = &to_interface;  //Rule to
    entry._rules.back()._type = RoutingTable::MPLS;
    entry._rules.back()._weight = weight;
    entry._rules.back()._ops.emplace_back();
    auto& op = entry._rules.back()._ops.back();
    op._op_label.set_value(q_type, interface_label_out, 0);
    op._op = type;

    std::ostream& warnings = std::cerr;
    table.sort();
    from_interface.table().merge(table, from_interface, warnings);
}

void re_labelling(){

}

void pair_and_assert(Interface* interface1, Interface* interface2){
    interface1->make_pairing(interface2);
    assert(interface1->match() == interface2);
    assert(interface2->match() == interface1);
}

Network construct_synthetic_network(int nesting = 1){
    int router_size = 5 * nesting;
    std::string router_name = "Router";
    std::vector<std::string> router_names;
    std::vector<std::unique_ptr<Router> > _routers;
    std::vector<const Interface*> _all_interfaces;
    Network::routermap_t _mapping;

    for(int i = 0; i < router_size; i++) {
        router_names.push_back(router_name + std::to_string(i));
    }

    int network_node = 0;
    bool nested = nesting > 1;
    bool fall_through = false;
    int last_nesting_begin = nesting * 5 - 5;

    for(int i = 0; i < router_size; i++, network_node++) {
        router_name = router_names[i];
        _routers.emplace_back(std::make_unique<Router>(i));
        Router &router = *_routers.back().get();
        router.add_name(router_name);
        auto res = _mapping.insert(router_name.c_str(), router_name.length());
        _mapping.get_data(res.second) = &router;
        switch (network_node) {
            case 1:
                router.get_interface(_all_interfaces, router_names[i - 1]);
                router.get_interface(_all_interfaces, router_names[i + 2]);
                break;
            case 2:
                router.get_interface(_all_interfaces, router_names[i + 1]);
                router.get_interface(_all_interfaces, router_names[i + 2]);
                if(nested){
                    router.get_interface(_all_interfaces, router_names[i + 6]);
                } else {
                    router.get_interface(_all_interfaces, router_names[i - 2]);
                }
                break;
            case 3:
                router.get_interface(_all_interfaces, router_names[i - 2]);
                router.get_interface(_all_interfaces, router_names[i + 1]);
                router.get_interface(_all_interfaces, router_names[i - 1]);
                if(i != 3) {
                    router.get_interface(_all_interfaces, router_names[i - 6]);
                }
                break;
            case 4:
                router.get_interface(_all_interfaces, router_names[i - 2]);
                router.get_interface(_all_interfaces, router_names[i - 1]);
                break;
            case 5:
                network_node = 0;   //Fall through
                if(i == last_nesting_begin){
                    nested = false;
                }
                fall_through = true;
            case 0:
                router.get_interface(_all_interfaces, router_names[i + 1]);
                if(nested){
                    router.get_interface(_all_interfaces, router_names[i + 5]);
                } else {
                    router.get_interface(_all_interfaces, router_names[i + 2]);
                }
                if(fall_through){
                    router.get_interface(_all_interfaces, router_names[i - 5]);
                    fall_through = false;
                }
                break;
            default:
                throw base_error("Something went wrong in the construction");
        }
    }
    Interface* passing_interface = nullptr;
    Interface* passing_interface1 = nullptr;
    Interface* to_interface = nullptr;
    Interface* interface1 = nullptr;
    Interface* interface2 = nullptr;

    for(int i = 0; 0 < nesting; nesting--, i+=5) {
        //Node0
        interface1 = _routers[i]->find_interface(router_names[i + 1]);
        interface2 = _routers[i + 1]->find_interface(router_names[i]);
        pair_and_assert(interface1, interface2);

        if(nesting > 1) {
            passing_interface = _routers[i]->find_interface(router_names[i + 5]);
            interface2 = _routers[i + 5]->find_interface(router_names[i]);
            pair_and_assert(passing_interface, interface2);

            //Add routing (1) 0 -> 5, (5) 0 -> 1
            //TODO, does not work with push?
            add_entry(*interface1, *passing_interface, aalwines::RoutingTable::SWAP, 0, 4, 4 );
            add_entry(*interface2, *interface1, aalwines::RoutingTable::SWAP, 0, 5, 22);

            //Add routing (0) 5 -> 6, (6) 5 -> 0
            interface1 = _routers[i + 5]->find_interface(router_names[i + 6]);
            add_entry(*interface2, *interface1, aalwines::RoutingTable::SWAP, 0, 4, 22);
            //TODO, does not work with POP?
            add_entry(*interface1, *interface2, aalwines::RoutingTable::SWAP, 0, 4, 4);

            //if next is not nested
            if(i + 10 == router_size){
                //Add routing (0) 5 -> 7, (7) 5 -> 0
                interface1 = _routers[i + 5]->find_interface(router_names[i + 7]);
                add_entry(*interface2, *interface1, aalwines::RoutingTable::SWAP, 0, 4, 6);
                //TODO, does not work with POP?
                add_entry(*interface1, *interface2, aalwines::RoutingTable::SWAP, 0, 5, 5);
            }
        } else {
            passing_interface = _routers[i]->find_interface(router_names[i + 2]);
            interface2 = _routers[i + 2]->find_interface(router_names[i]);
            pair_and_assert(passing_interface, interface2);
            //Add routings 2 0 -> 1, 1 0 -> 2 ||
            //Add routings 7 5 -> 6, 6 5 -> 7
            add_entry(*passing_interface, *interface1, aalwines::RoutingTable::SWAP, 0, 5, 22);
            add_entry(*interface1, *passing_interface, aalwines::RoutingTable::SWAP, 0, 4, 6);
        }

        //Node1
        passing_interface = _routers[i + 1]->find_interface(router_names[i + 0]);

        interface1 = _routers[i + 1]->find_interface(router_names[i + 3]);
        interface2 = _routers[i + 3]->find_interface(router_names[i + 1]);
        pair_and_assert(interface1, interface2);

        //Add entries 0 - 3, 3 - 0
        add_entry(*passing_interface, *interface1, aalwines::RoutingTable::SWAP, 0, 6, 7);
        add_entry(*interface1, *passing_interface, aalwines::RoutingTable::SWAP, 0, 22, 4);

        //Node2
        to_interface = _routers[i + 2]->find_interface(router_names[i + 3]);
        interface2 = _routers[i + 3]->find_interface(router_names[i + 2]);
        pair_and_assert(to_interface, interface2);

        interface1 = _routers[i + 2]->find_interface(router_names[i + 4]);
        interface2 = _routers[i + 4]->find_interface(router_names[i + 2]);
        pair_and_assert(interface1, interface2);

        //Add entries 4 2 -> 3, 3 2 -> 4
        add_entry(*interface1, *to_interface, aalwines::RoutingTable::SWAP, 0, 33, 10);
        add_entry(*to_interface, *interface1, aalwines::RoutingTable::SWAP, 0, 7, 22);

        if(nesting > 1) {
            interface1 = _routers[i + 2]->find_interface(router_names[i + 8]);
            interface2 = _routers[i + 8]->find_interface(router_names[i + 2]);
            pair_and_assert(interface1, interface2);

            passing_interface = _routers[i + 2]->find_interface(router_names[i + 3]);
            passing_interface1 = _routers[i + 2]->find_interface(router_names[i + 4]);
            //Add entries 3 2 -> 8, 8 2 -> 3
            //TODO; Should work with PUSH
            add_entry(*passing_interface, *interface1, aalwines::RoutingTable::SWAP, 0, 7, 10);

            add_entry(*interface1, *passing_interface, aalwines::RoutingTable::SWAP, 0, 4, 10);

            //Add entries 4 2 -> 8, 8 2 -> 4
            //TODO; Should work with PUSH
            add_entry(*passing_interface1, *interface1, aalwines::RoutingTable::SWAP, 0, 33, 10);
            add_entry(*interface1, *passing_interface1, aalwines::RoutingTable::SWAP, 0, 4, 22);
        } else {
            passing_interface = _routers[i + 2]->find_interface(router_names[i + 0]);
            passing_interface1 = _routers[i + 2]->find_interface(router_names[i + 4]);

            //Add entries 0 2 -> 3, 3 2 -> 0
            add_entry(*passing_interface, *to_interface, aalwines::RoutingTable::SWAP, 0, 6, 7);
            add_entry(*to_interface, *passing_interface, aalwines::RoutingTable::SWAP, 0, 6, 5);

            //Add entries 0 2 -> 4, 4 2 -> 0
            add_entry(*passing_interface, *passing_interface1, aalwines::RoutingTable::SWAP, 0, 6, 22);
            add_entry(*passing_interface1, *passing_interface, aalwines::RoutingTable::SWAP, 0, 6, 5);
        }

        //Node3
        passing_interface = _routers[i + 3]->find_interface(router_names[i + 1]);
        interface1 = _routers[i + 3]->find_interface(router_names[i + 4]);
        interface2 = _routers[i + 4]->find_interface(router_names[i + 3]);
        pair_and_assert(interface1, interface2);

        //Add entries 1 - 4, 4 - 1
        add_entry(*passing_interface, *interface1, aalwines::RoutingTable::SWAP, 0, 7, 9);
        add_entry(*interface1, *passing_interface, aalwines::RoutingTable::SWAP, 0, 33, 22);

        passing_interface1 = _routers[i + 3]->find_interface(router_names[i + 2]);
        //Add entries 2 3 -> 4, 4 - 2
        add_entry(*passing_interface1, *interface1, aalwines::RoutingTable::SWAP, 0, 7, 9);
        add_entry(*interface1, *passing_interface1, aalwines::RoutingTable::SWAP, 0, 33, 7);

        //Add entries 1 3 -> 2, 2 3 -> 1
        add_entry(*passing_interface, *passing_interface1, aalwines::RoutingTable::SWAP, 0, 7, 7);
        add_entry(*passing_interface1, *passing_interface, aalwines::RoutingTable::SWAP, 0, 10, 22);

        if(nesting > 1){
            interface1 = _routers[i + 8]->find_interface(router_names[i + 6]);
            passing_interface = _routers[i + 8]->find_interface(router_names[i + 7]);
            interface2 = _routers[i + 8]->find_interface(router_names[i + 9]);
            to_interface = _routers[i + 8]->find_interface(router_names[i + 2]);

            //Add entries 6 8 -> 2, 2 8 -> 6
            //TODO; POP?
            add_entry(*interface1, *to_interface, aalwines::RoutingTable::SWAP, 0, 7, 4);
            add_entry(*to_interface, *interface1, aalwines::RoutingTable::SWAP, 0, 10, 22);

            //Add entries 9 8 -> 2, 2 8 -> 9
            //TODO; POP?
            add_entry(*interface2, *to_interface, aalwines::RoutingTable::SWAP, 0, 12, 4);
            add_entry(*to_interface, *interface2, aalwines::RoutingTable::SWAP, 0, 7, 9);

            //Add entries 7 8 -> 2, 2 8 -> 7
            //TODO; POP?
            add_entry(*passing_interface, *to_interface, aalwines::RoutingTable::SWAP, 0, 7, 4);
            add_entry(*to_interface, *passing_interface, aalwines::RoutingTable::SWAP, 0, 10, 6);
        }

        //Node4
        interface1 = _routers[i + 4]->find_interface(router_names[i + 2]);
        interface2 = _routers[i + 4]->find_interface(router_names[i + 3]);
        //Add entries 2 - 3, 3 - 2
        add_entry(*interface1, *interface2, aalwines::RoutingTable::SWAP, 0, 22, 12);
        add_entry(*interface2, *interface1, aalwines::RoutingTable::SWAP, 0, 9, 33);

        //No more edges to add
    }
    Router::add_null_router(_routers, _all_interfaces, _mapping); //Last router

    return Network(std::move(_mapping), std::move(_routers), std::move(_all_interfaces));
}

BOOST_AUTO_TEST_CASE(NetworkConstruction) {
    Network synthetic_network = construct_synthetic_network(2);
    //Network synthetic_network2 = construct_synthetic_network();
    //synthetic_network.manipulate_network(0, 2, synthetic_network2, 0, synthetic_network2.size() - 2);

    //synthetic_network.print_dot(std::cout);

    Builder builder(synthetic_network);
    {
        std::string query(
                "<[6]> [.#Router1] .* [Router3#.] <.> 0 OVER \n"
                "<.> [Router0#.] .* [.#Router4] <.> 0 OVER \n"
                "<.> [Router0#.] .* [Router2#.] <.> 0 OVER \n"
                "<.> [Router0#.] .* [Router3#.] <.> 0 OVER \n"
                "<.> [.#Router0] .* [Router7#.] <.> 0 OVER \n"
                "<.> [Router1#.] .* [.#Router7] <.> 0 OVER \n"
                "<.> [Router0#.] .* [.#Router7] <.> 0 OVER \n"
                "<.> [.#Router0] .* [Router9#.] <.> 0 OVER \n"
                "<.> [.#Router5] .* [Router7#.] <.> 0 OVER \n"
                "<[12]> [.#Router8] .* [Router2#.] <.> 0 OVER \n"
                "<.> [Router4#.] .* [Router2#.] <.> 0 OVER \n"
                       );
        //Adapt to existing query parser
        std::istringstream qstream(query);
        builder.do_parse(qstream);
    }

    size_t query_no = 0;

    pdaaal::Solver_Adapter solver;
    for(auto& q : builder._result) {
        query_no++;
        std::vector<Query::mode_t> modes{q.approximation()};

        bool was_dual = q.approximation() == Query::DUAL;
        if (was_dual)
            modes = std::vector<Query::mode_t>{Query::OVER, Query::UNDER};
        std::pair<size_t, size_t> reduction;
        utils::outcome_t result = utils::MAYBE;
        std::vector<pdaaal::TypedPDA<Query::label_t>::tracestate_t> trace;
        std::stringstream proof;

        size_t tos = 0;
        bool no_ip_swap = false;
        bool get_trace = true;

        for (auto m : modes) {
            q.set_approximation(m);
            NetworkPDAFactory factory(q, synthetic_network, no_ip_swap);
            auto pda = factory.compile();
            reduction = pdaaal::Reducer::reduce(pda, tos, pda.initial(), pda.terminal());
            bool need_trace = was_dual || get_trace;

            auto solver_result1 = solver.post_star<pdaaal::Trace_Type::Any>(pda);
            bool engine_outcome = solver_result1.first;
            if (need_trace && engine_outcome) {
                trace = solver.get_trace(pda, std::move(solver_result1.second));
                if (factory.write_json_trace(proof, trace))
                    result = utils::YES;
            }
            if (q.number_of_failures() == 0)
                result = engine_outcome ? utils::YES : utils::NO;

            if (result == utils::MAYBE && m == Query::OVER && !engine_outcome)
                result = utils::NO;
            if (result != utils::MAYBE)
                break;
        }
        std::cout << "\t\"Q" << query_no << "\" : {\n\t\t\"result\":";
        switch (result) {
            case utils::MAYBE:
                std::cout << "null";
                break;
            case utils::NO:
                std::cout << "false";
                break;
            case utils::YES:
                std::cout << "true";
                break;
        }
        std::cout << ",\n";
        std::cout << "\t\t\"reduction\":[" << reduction.first << ", " << reduction.second << "]";
        if (get_trace && result == utils::YES) {
            std::cout << ",\n\t\t\"trace\":[\n";
            std::cout << proof.str();
            std::cout << "\n\t\t]";
        }
        std::cout << "\n\t}";
        if (query_no != builder._result.size())
            std::cout << ",";
        std::cout << "\n";
        std::cout << "\n}}" << std::endl;
    }
    BOOST_CHECK_EQUAL(true, true);
}