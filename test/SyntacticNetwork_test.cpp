//
// Created by Morten on 18-03-2020.
//

#define BOOST_TEST_MODULE SyntacticNetwork

#include <boost/test/unit_test.hpp>
#include <aalwines/model/Network.h>

using namespace aalwines;

Network manipulate_network(Network synthetic_network, int start_router_index, int end_router_index, Network nested_synthetic_network){
    if(!synthetic_network.size() || !nested_synthetic_network.size()) throw base_error("Networks must be defined");

    //Modify start and end routers
    Router* router_start = synthetic_network.get_router(start_router_index);
    Router* router_end = synthetic_network.get_router(end_router_index);
    Router* nested_router_start = nested_synthetic_network.get_start_router();
    Router* nested_router_end = nested_synthetic_network.get_end_router();
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

    //Map all routers into one network
    routers.insert((std::end(routers) - 1), std::make_move_iterator(std::begin(nested_routers)), std::make_move_iterator(std::end(nested_routers)-1));

    interfaces.insert(interfaces.end(), nested_synthetic_network.all_interfaces().begin(),
          nested_synthetic_network.all_interfaces().end());   //Map all interfaces

    Network::routermap_t mapping;
    //Network::routermap_t mapping = synthetic_network.get_mapping();

    return Network(std::move(mapping), std::move(routers), std::move(interfaces));
}

Router* get_router(std::string router_name, Network::routermap_t& _mapping){
    auto res = _mapping.exists(router_name.c_str(), router_name.length());
    return _mapping.get_data(res.second);
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

Network construct_synthetic_network(){
    int router_size = 5;
    std::string router_name = "Router";
    std::vector<std::string> router_names;
    using routermap_t = ptrie::map<char, Router *>;
    std::vector<std::unique_ptr<Router> > _routers;
    std::vector<const Interface*> _all_interfaces;
    Network::routermap_t _mapping;

    for(int i = 0; i < router_size; i++) {
        router_names.push_back(router_name + std::to_string(i));
    }

    for(int i = 0; i < router_size; i++) {
        router_name = router_names[i];
        _routers.emplace_back(std::make_unique<Router>(i));
        Router &router = *_routers.back().get();
        router.add_name(router_name);
        auto res = _mapping.insert(router_name.c_str(), router_name.length());
        _mapping.get_data(res.second) = &router;
        switch (i) {
            case 0:
                router.get_interface(_all_interfaces, router_names[i + 1]);
                router.get_interface(_all_interfaces, router_names[i + 2]);
                break;
            case 1:
                router.get_interface(_all_interfaces, router_names[i - 1]);
                router.get_interface(_all_interfaces, router_names[i + 2]);
                break;
            case 2:
                router.get_interface(_all_interfaces, router_names[i + 1]);
                router.get_interface(_all_interfaces, router_names[i + 2]);
                router.get_interface(_all_interfaces, router_names[i - 2]);
                break;
            case 3:
                router.get_interface(_all_interfaces, router_names[i - 2]);
                router.get_interface(_all_interfaces, router_names[i + 1]);
                router.get_interface(_all_interfaces, router_names[i - 1]);
                break;
            case 4:
                router.get_interface(_all_interfaces, router_names[i - 2]);
                router.get_interface(_all_interfaces, router_names[i - 1]);
                break;
            default:
                throw base_error("Something went wrong in the construction");
        }
    }
    Router* router0 = get_router(router_names[0], _mapping);
    Router* router1 = get_router(router_names[1], _mapping);
    Router* router2 = get_router(router_names[2], _mapping);
    Router* router3 = get_router(router_names[3], _mapping);
    Router* router4 = get_router(router_names[4], _mapping);

    RoutingTable table;
    int interface_label = 4;
    size_t weight = 0;
    Query::type_t type = Query::MPLS;

    Interface* interface1 = nullptr;
    Interface* interface2 = nullptr;

    //Node0
    interface1 = router0->find_interface(router_names[1]);
    interface2 = router1->find_interface(router_names[0]);
    interface1->make_pairing(interface2);
    interface2->make_pairing(interface1);
    add_entry(table, *interface1, *interface2, type, weight, interface_label);

    interface1 = router0->find_interface(router_names[2]);
    interface2 = router2->find_interface(router_names[0]);
    interface1->make_pairing(interface2);
    interface2->make_pairing(interface1);
    add_entry(table, *interface1, *interface2, type, weight, interface_label);

    //Node1
    interface1 = router1->find_interface(router_names[3]);
    interface2 = router3->find_interface(router_names[1]);
    interface1->make_pairing(interface2);
    interface2->make_pairing(interface1);
    add_entry(table, *interface1, *interface2, type, weight, interface_label);

    //Node2
    interface1 = router2->find_interface(router_names[3]);
    interface2 = router3->find_interface(router_names[2]);
    interface1->make_pairing(interface2);
    interface2->make_pairing(interface1);
    add_entry(table, *interface1, *interface2, type, weight, interface_label);
    interface1 = router2->find_interface(router_names[4]);
    interface2 = router4->find_interface(router_names[2]);
    interface1->make_pairing(interface2);
    interface2->make_pairing(interface1);
    add_entry(table, *interface1, *interface2, type, weight, interface_label);

    //Node3
    interface1 = router3->find_interface(router_names[4]);
    interface2 = router4->find_interface(router_names[3]);
    interface1->make_pairing(interface2);
    interface2->make_pairing(interface1);
    add_entry(table, *interface1, *interface2, type, weight, interface_label);

    //Node4
    //No more edges to add

    Router::add_null_router(_routers, _all_interfaces, _mapping); //Last router
    return Network(std::move(_mapping), std::move(_routers), std::move(_all_interfaces));
}

BOOST_AUTO_TEST_CASE(NetworkConstruction) {
    Network synthetic_network = construct_synthetic_network();
    Network synthetic_network2 = construct_synthetic_network();
    synthetic_network = manipulate_network(std::move(synthetic_network), 0, 2, std::move(synthetic_network2));
    synthetic_network.print_dot(std::cout);

    BOOST_CHECK_EQUAL(true, true);
}