//
// Created by Morten on 18-03-2020.
//

#define BOOST_TEST_MODULE SyntacticNetwork

#include <boost/test/unit_test.hpp>
#include <aalwines/model/Network.h>

using namespace aalwines;

Network manipulate_network(Network synthetic_network, int start_router_index, int end_router_index, Network nested_synthetic_network,
        int start_router_index2, int end_router_index2){

    if(!synthetic_network.size() || !nested_synthetic_network.size()) throw base_error("Networks must be defined");

    //Modify start and end routers
    Router* router_start = synthetic_network.get_router(start_router_index);
    Router* router_end = synthetic_network.get_router(end_router_index);
    Router* nested_router_start = nested_synthetic_network.get_router(start_router_index2);
    Router* nested_router_end = nested_synthetic_network.get_router(end_router_index2);
    //Remove pairing link
    Interface* interface1 = router_start->find_interface(router_end->name());
    Interface* interface2 = router_end->find_interface(router_start->name());
    router_start->remove_interface(interface1);
    router_end->remove_interface(interface2);

    interface1 = router_start->get_interface(synthetic_network.all_interfaces(), nested_router_start->name());
    interface2 = nested_router_start->get_interface(synthetic_network.all_interfaces(), router_start->name());
    interface1->make_pairing(interface2);
    interface2->make_pairing(interface1);

    interface1 = router_end->get_interface(synthetic_network.all_interfaces(), nested_router_end->name());
    interface2 = nested_router_end->get_interface(synthetic_network.all_interfaces(), router_end->name());
    interface1->make_pairing(interface2);
    interface2->make_pairing(interface1);

    //Construct new network
    std::vector<std::unique_ptr<Router>> routers = synthetic_network.get_all_routers();
    std::vector<std::unique_ptr<Router>> nested_routers = nested_synthetic_network.get_all_routers();
    std::vector<const Interface*> interfaces = synthetic_network.all_interfaces();
    routers.pop_back(); //Remove NULL router

    //Give routers distinct names
    int name_iterator = 0;
    std::string name = routers.back()->name();
    for (auto& e : nested_routers){
        if(std::find_if(routers.begin(), routers.end(),
                [&](auto& rh){ return rh.get()->name() == e.get()->name(); } )->get() != routers.end()->get()) {
            std::string old_name = e.get()->name();
            e.get()->change_name(name + std::to_string(name_iterator++));
        }
    }

    //Map all routers into one network
    routers.insert(std::end(routers), std::make_move_iterator(std::begin(nested_routers)), std::make_move_iterator(std::end(nested_routers)-1));

    interfaces.insert(interfaces.end(), nested_synthetic_network.all_interfaces().begin(),
          nested_synthetic_network.all_interfaces().end());   //Map all interfaces

    Network::routermap_t mapping;
    //Network::routermap_t mapping = synthetic_network.get_mapping();

    return Network(std::move(mapping), std::move(routers), std::move(interfaces));
}

void add_entry(RoutingTable table, const Interface& interface1, Interface interface2, Query::type_t type, int weight, int interface_label) {
    auto& entry = table.push_entry();
    entry._ingoing = &interface1;           //From interface
    type = Query::MPLS;
    entry._top_label.set_value(type, interface_label, 0);

    entry._rules.emplace_back();
    entry._rules.back()._via = &interface2;  //Rule to
    entry._rules.back()._type = RoutingTable::MPLS;
    entry._rules.back()._weight = weight;
    entry._rules.back()._ops.emplace_back();
    auto& op = entry._rules.back()._ops.back();
    op._op = RoutingTable::SWAP;
}

Network construct_synthetic_network(int nesting = 1){
    int router_size = 5 * nesting;
    std::string router_name = "Router";
    std::vector<std::string> router_names;
    using routermap_t = ptrie::map<char, Router *>;
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
    RoutingTable table;
    int interface_label = 4;
    size_t weight = 0;
    Query::type_t type = Query::MPLS;

    Interface* interface1 = nullptr;
    Interface* interface2 = nullptr;

    for(int i = 0; 0 < nesting; nesting--, i+=5) {
        //Node0
        interface1 = _routers[i].get()->find_interface(router_names[i + 1]);
        interface2 = _routers[i + 1].get()->find_interface(router_names[i]);
        interface1->make_pairing(interface2);
        interface2->make_pairing(interface1);
        add_entry(table, *interface1, *interface2, type, weight, interface_label);

        if(nesting > 1) {
            interface1 = _routers[i].get()->find_interface(router_names[i + 5]);
            interface2 = _routers[i + 5].get()->find_interface(router_names[i]);
            interface1->make_pairing(interface2);
            interface2->make_pairing(interface1);
            add_entry(table, *interface1, *interface2, type, weight, interface_label);
        } else {
            interface1 = _routers[i].get()->find_interface(router_names[i + 2]);
            interface2 = _routers[i + 2].get()->find_interface(router_names[i]);
            interface1->make_pairing(interface2);
            interface2->make_pairing(interface1);
            add_entry(table, *interface1, *interface2, type, weight, interface_label);
        }

        //Node1
        interface1 = _routers[i + 1].get()->find_interface(router_names[i + 3]);
        interface2 = _routers[i + 3].get()->find_interface(router_names[i + 1]);
        interface1->make_pairing(interface2);
        interface2->make_pairing(interface1);
        add_entry(table, *interface1, *interface2, type, weight, interface_label);

        //Node2
        interface1 = _routers[i + 2].get()->find_interface(router_names[i + 3]);
        interface2 = _routers[i + 3].get()->find_interface(router_names[i + 2]);
        interface1->make_pairing(interface2);
        interface2->make_pairing(interface1);
        add_entry(table, *interface1, *interface2, type, weight, interface_label);
        interface1 = _routers[i + 2].get()->find_interface(router_names[i + 4]);
        interface2 = _routers[i + 4].get()->find_interface(router_names[i + 2]);
        interface1->make_pairing(interface2);
        interface2->make_pairing(interface1);
        add_entry(table, *interface1, *interface2, type, weight, interface_label);

        if(nesting > 1) {
            interface1 = _routers[i + 2].get()->find_interface(router_names[i + 8]);
            interface2 = _routers[i + 8].get()->find_interface(router_names[i + 2]);
            interface1->make_pairing(interface2);
            interface2->make_pairing(interface1);
            add_entry(table, *interface1, *interface2, type, weight, interface_label);
        }

        //Node3
        interface1 = _routers[i + 3].get()->find_interface(router_names[i + 4]);
        interface2 = _routers[i + 4].get()->find_interface(router_names[i + 3]);
        interface1->make_pairing(interface2);
        interface2->make_pairing(interface1);
        add_entry(table, *interface1, *interface2, type, weight, interface_label);

        //Node4
        //No more edges to add
    }
    Router::add_null_router(_routers, _all_interfaces, _mapping); //Last router
    return Network(std::move(_mapping), std::move(_routers), std::move(_all_interfaces));
}

BOOST_AUTO_TEST_CASE(NetworkConstruction) {
    Network synthetic_network = construct_synthetic_network(3);
    Network synthetic_network2 = construct_synthetic_network();
    synthetic_network = manipulate_network(std::move(synthetic_network), 0,
            2, std::move(synthetic_network2), 0, synthetic_network2.size() - 2);
    synthetic_network.print_dot(std::cout);

    BOOST_CHECK_EQUAL(true, true);
}