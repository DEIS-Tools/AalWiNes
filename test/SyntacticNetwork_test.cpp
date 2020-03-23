//
// Created by Morten on 18-03-2020.
//

#define BOOST_TEST_MODULE SyntacticNetwork

#include <boost/test/unit_test.hpp>
#include <aalwines/model/Network.h>

using namespace aalwines;


Network createmapping(int iterations = 2) {
    int router_size = iterations * 5;
    std::string router_name = "Router";
    std::vector<std::string> router_names;

    using routermap_t = ptrie::map<char, Router *>;
    routermap_t _mapping;
    std::vector<std::unique_ptr<Router>> _routers;
    std::vector<const Interface *> _all_interfaces;

    //Synthetic Network Design.
    //Routers
    for(int i = 0; i < router_size; i++) {
        router_names.push_back(router_name + std::to_string(i));
    }
    int iteration_nr = 0;
    for(int i = 0; i < router_size; i++, iteration_nr++){
        router_name = router_names[i];
        _routers.emplace_back(std::make_unique<Router>(i));
        Router &router = *_routers.back().get();
        router.add_name(router_name);
        auto res = _mapping.insert(router_name.c_str(), router_name.length());
        _mapping.get_data(res.second) = &router;

        //Was in test net but not needed here?
        //router.get_interface(_all_interfaces, router_name); // Will add the corresponding interface for the router
        switch (iteration_nr) {
            case 0:
                router.get_interface(_all_interfaces, router_names[i + 1]);
                router.get_interface(_all_interfaces, router_names[i + 2]);
                break;
            case 1:
                router.get_interface(_all_interfaces, router_names[i - 1]);
                router.get_interface(_all_interfaces, router_names[i + 2]);
                break;
            case 2:
                router.get_interface(_all_interfaces, router_names[i - 2]);
                router.get_interface(_all_interfaces, router_names[i + 1]);
                router.get_interface(_all_interfaces, router_names[i + 2]);
                break;
            case 3:
                router.get_interface(_all_interfaces, router_names[i - 2]);
                router.get_interface(_all_interfaces, router_names[i + 1]);
                router.get_interface(_all_interfaces, router_names[i - 1]);
                if(iterations > 1){
                    if(i == 3)
                        router.get_interface(_all_interfaces, router_names[i + 5]);
                    if(i > 3)
                        router.get_interface(_all_interfaces, router_names[i - 5]);
                }

                break;
            case 4:
                router.get_interface(_all_interfaces, router_names[i - 2]);
                router.get_interface(_all_interfaces, router_names[i - 1]);
                break;
            case 5: //Nesting
                router.get_interface(_all_interfaces, router_names[i - 5]);
                router.get_interface(_all_interfaces, router_names[i + 2]);
                router.get_interface(_all_interfaces, router_names[i + 1]);

                iteration_nr -= 5; //Repeat construction

                {
                    auto res = _mapping.exists(router_names[i - 5].c_str(), router_names[i - 5].length());
                    Router* router = _mapping.get_data(res.second);
                    router->get_interface(_all_interfaces, router_names[i]);
                }

                break;
            default:
                throw base_error("Something went wrong in the construction");
        }
    }

    //Routing table
    RoutingTable table;
    int interface_label;
    size_t weight = 0;
    Query::type_t type;

    //Build routers -> topo.xml
    //foreach router:
    iteration_nr = 0;
    for (int i = 0; i < router_size; i++, iteration_nr++) {
        router_name = router_names[i];
        auto res = _mapping.insert(router_name.c_str(), router_name.length());
        Router* router = _mapping.get_data(res.second);



        //Interfaces
        Interface *current = nullptr;
        Interface *inter1 = nullptr;
        Interface *inter2 = nullptr;
        Interface *inter3 = nullptr;

        switch (iteration_nr) {
            case 0: {
                res = _mapping.insert(router_names[i + 1].c_str(), router_names[i + 1].length());
                auto router1 = _mapping.get_data(res.second);
                res = _mapping.insert(router_names[i + 2].c_str(), router_names[i + 2].length());
                auto router2 = _mapping.get_data(res.second);

                current = router1->find_interface(router_name);
                inter1 = router->find_interface(router_names[i + 1]);

                current->make_pairing(inter1);

                current = router2->find_interface(router_name);
                inter2 = router->find_interface(router_names[i + 2]);

                current->make_pairing(inter2);

                // Routing Table
                //Destinations
                auto &entry = table.push_entry();
                interface_label = 4;
                entry._ingoing = current;           //From interface
                type = Query::MPLS;
                entry._top_label.set_value(type, interface_label, 0);

                //Add rules
                entry._rules.emplace_back();
                entry._rules.back()._via = inter1;  //Rule to
                entry._rules.back()._type = RoutingTable::MPLS;
                entry._rules.back()._weight = weight;
                entry._rules.back()._ops.emplace_back();
                auto &op = entry._rules.back()._ops.back();
                op._op = RoutingTable::SWAP;
            }
            {
                //Next Destination
                auto& entry = table.push_entry();
                interface_label = 4;
                entry._ingoing = current;           //From interface
                type = Query::MPLS;
                entry._top_label.set_value(type, interface_label, 0);

                entry._rules.emplace_back();
                entry._rules.back()._via = inter2;  //Rule to
                entry._rules.back()._type = RoutingTable::MPLS;
                entry._rules.back()._weight = weight;
                entry._rules.back()._ops.emplace_back();
                auto& op = entry._rules.back()._ops.back();
                op._op = RoutingTable::SWAP;
                ++weight;
                break;
            }
            case 1: {
                res = _mapping.insert(router_names[i - 1].c_str(), router_names[i - 1].length());
                auto router1 = _mapping.get_data(res.second);
                res = _mapping.insert(router_names[i + 2].c_str(), router_names[i + 2].length());
                auto router2 = _mapping.get_data(res.second);

                current = router1->find_interface(router_name);
                inter1 = router->find_interface(router_names[i - 1]);

                current->make_pairing(inter1);

                current = router2->find_interface(router_name);
                inter2 = router->find_interface(router_names[i + 2]);

                current->make_pairing(inter2);


                // Routing Table
                //Destinations
                auto &entry = table.push_entry();
                interface_label = 4;
                entry._ingoing = current;           //From interface
                type = Query::MPLS;
                entry._top_label.set_value(type, interface_label, 0);

                entry._rules.emplace_back();
                entry._rules.back()._via = inter1;  //Rule to
                entry._rules.back()._type = RoutingTable::MPLS;
                entry._rules.back()._weight = weight;
                entry._rules.back()._ops.emplace_back();
                auto &op = entry._rules.back()._ops.back();
                op._op = RoutingTable::SWAP;
            }
            {
                //Next Destination
                auto& entry = table.push_entry();
                interface_label = 4;
                entry._ingoing = current;           //From interface
                type = Query::MPLS;
                entry._top_label.set_value(type, interface_label, 0);

                entry._rules.emplace_back();
                entry._rules.back()._via = inter2;  //Rule to
                entry._rules.back()._type = RoutingTable::MPLS;
                entry._rules.back()._weight = weight;
                entry._rules.back()._ops.emplace_back();
                auto& op = entry._rules.back()._ops.back();
                op._op = RoutingTable::SWAP;
                ++weight;
                break;
            }
            case 2: {
                res = _mapping.insert(router_names[i - 2].c_str(), router_names[i - 2].length());
                auto router1 = _mapping.get_data(res.second);

                res = _mapping.insert(router_names[i + 2].c_str(), router_names[i + 2].length());
                auto router2 = _mapping.get_data(res.second);

                res = _mapping.insert(router_names[i + 1].c_str(), router_names[i + 1].length());
                auto router3 = _mapping.get_data(res.second);

                current = router1->find_interface(router_name);
                inter1 = router->find_interface(router_names[i - 2]);

                current->make_pairing(inter1);

                current = router2->find_interface(router_name);
                inter2 = router->find_interface(router_names[i + 2]);

                current->make_pairing(inter2);

                current = router3->find_interface(router_name);
                inter3 = router->find_interface(router_names[i + 1]);
                current->make_pairing(inter3);

                // Routing Table
                //Destinations
                auto &entry = table.push_entry();
                interface_label = 4;
                entry._ingoing = current;           //From interface
                type = Query::MPLS;
                entry._top_label.set_value(type, interface_label, 0);

                entry._rules.emplace_back();
                entry._rules.back()._via = inter1;  //Rule to
                entry._rules.back()._type = RoutingTable::MPLS;
                entry._rules.back()._weight = weight;
                entry._rules.back()._ops.emplace_back();
                auto &op = entry._rules.back()._ops.back();
                op._op = RoutingTable::SWAP;
            }
            {
                //Next Destination
                auto &entry = table.push_entry();
                interface_label = 4;
                entry._ingoing = current;           //From interface
                type = Query::MPLS;
                entry._top_label.set_value(type, interface_label, 0);

                entry._rules.emplace_back();
                entry._rules.back()._via = inter2;  //Rule to
                entry._rules.back()._type = RoutingTable::MPLS;
                entry._rules.back()._weight = weight;
                entry._rules.back()._ops.emplace_back();
                auto &op = entry._rules.back()._ops.back();
                op._op = RoutingTable::SWAP;
            }
            {
                //Next Destination
                auto& entry = table.push_entry();
                interface_label = 4;
                entry._ingoing = current;           //From interface
                type = Query::MPLS;
                entry._top_label.set_value(type, interface_label, 0);

                entry._rules.emplace_back();
                entry._rules.back()._via = inter3;  //Rule to
                entry._rules.back()._type = RoutingTable::MPLS;
                entry._rules.back()._weight = weight;
                entry._rules.back()._ops.emplace_back();
                auto& op = entry._rules.back()._ops.back();
                op._op = RoutingTable::SWAP;
                ++weight;
                break;
            }
            case 3: {
                res = _mapping.insert(router_names[i - 2].c_str(), router_names[i - 2].length());
                auto router1 = _mapping.get_data(res.second);
                res = _mapping.insert(router_names[i - 1].c_str(), router_names[i - 1].length());
                auto router2 = _mapping.get_data(res.second);
                res = _mapping.insert(router_names[i - 2].c_str(), router_names[i - 2].length());
                auto router3 = _mapping.get_data(res.second);

                current = router1->find_interface(router_name);
                inter1 = router->find_interface(router_names[i - 2]);

                current->make_pairing(inter1);

                current = router2->find_interface(router_name);
                inter2 = router->find_interface(router_names[i - 1]);

                current->make_pairing(inter2);

                current = router3->find_interface(router_name);
                inter3 = router->find_interface(router_names[i - 2]);
                current->make_pairing(inter3);

                if(iterations > 1 && i < 5 * (iterations-1)){
                    res = _mapping.insert(router_names[i + 5].c_str(), router_names[i + 5].length());
                    auto router4 = _mapping.get_data(res.second);

                    current = router4->find_interface(router_name);
                    auto inter4 = router->find_interface(router_names[i + 5]);
                    current->make_pairing(inter4);
                }
                else {
                    res = _mapping.insert(router_names[i - 5].c_str(), router_names[i - 5].length());
                    auto router4 = _mapping.get_data(res.second);

                    current = router4->find_interface(router_name);
                    auto inter4 = router->find_interface(router_names[i - 5]);
                    current->make_pairing(inter4);
                }

                // Routing Table
                //Destinations
                auto &entry = table.push_entry();
                interface_label = 4;
                entry._ingoing = current;           //From interface
                type = Query::MPLS;
                entry._top_label.set_value(type, interface_label, 0);

                entry._rules.emplace_back();
                entry._rules.back()._via = inter1;  //Rule to
                entry._rules.back()._type = RoutingTable::MPLS;
                entry._rules.back()._weight = weight;
                entry._rules.back()._ops.emplace_back();
                auto &op = entry._rules.back()._ops.back();
                op._op = RoutingTable::SWAP;
            }
                {
                    //Next Destination
                    auto &entry = table.push_entry();
                    interface_label = 4;
                    entry._ingoing = current;           //From interface
                    type = Query::MPLS;
                    entry._top_label.set_value(type, interface_label, 0);

                    entry._rules.emplace_back();
                    entry._rules.back()._via = inter2;  //Rule to
                    entry._rules.back()._type = RoutingTable::MPLS;
                    entry._rules.back()._weight = weight;
                    entry._rules.back()._ops.emplace_back();
                    auto &op = entry._rules.back()._ops.back();
                    op._op = RoutingTable::SWAP;
                }
                {
                    //Next Destination
                    auto& entry = table.push_entry();
                    interface_label = 4;
                    entry._ingoing = current;           //From interface
                    type = Query::MPLS;
                    entry._top_label.set_value(type, interface_label, 0);

                    entry._rules.emplace_back();
                    entry._rules.back()._via = inter3;  //Rule to
                    entry._rules.back()._type = RoutingTable::MPLS;
                    entry._rules.back()._weight = weight;
                    entry._rules.back()._ops.emplace_back();
                    auto& op = entry._rules.back()._ops.back();
                    op._op = RoutingTable::SWAP;
                    ++weight;
                    break;
                }
            case 4: {
                res = _mapping.insert(router_names[i - 2].c_str(), router_names[i - 2].length());
                auto router1 = _mapping.get_data(res.second);
                res = _mapping.insert(router_names[i - 1].c_str(), router_names[i - 1].length());
                auto router2 = _mapping.get_data(res.second);

                current = router1->find_interface(router_name);
                inter1 = router->find_interface(router_names[i - 2]);

                current->make_pairing(inter1);

                current = router2->find_interface(router_name);
                inter2 = router->find_interface(router_names[i - 1]);

                current->make_pairing(inter2);

                // Routing Table
                //Destinations
                auto &entry = table.push_entry();
                interface_label = 4;
                entry._ingoing = current;           //From interface
                type = Query::MPLS;
                entry._top_label.set_value(type, interface_label, 0);

                entry._rules.emplace_back();
                entry._rules.back()._via = inter1;  //Rule to
                entry._rules.back()._type = RoutingTable::MPLS;
                entry._rules.back()._weight = weight;
                entry._rules.back()._ops.emplace_back();
                auto &op = entry._rules.back()._ops.back();
                op._op = RoutingTable::SWAP;
            }
            {
                //Next Destination
                auto& entry = table.push_entry();
                interface_label = 4;
                entry._ingoing = current;           //From interface
                type = Query::MPLS;
                entry._top_label.set_value(type, interface_label, 0);

                entry._rules.emplace_back();
                entry._rules.back()._via = inter2;  //Rule to
                entry._rules.back()._type = RoutingTable::MPLS;
                entry._rules.back()._weight = weight;
                entry._rules.back()._ops.emplace_back();
                auto& op = entry._rules.back()._ops.back();
                op._op = RoutingTable::SWAP;
            }
                ++weight;
                break;
            case 5: {

                res = _mapping.exists(router_names[i - 5].c_str(), router_names[i - 5].length());
                auto router1 = _mapping.get_data(res.second);

                res = _mapping.exists(router_names[i + 2].c_str(), router_names[i + 2].length());
                auto router2 = _mapping.get_data(res.second);

                res = _mapping.exists(router_names[i + 1].c_str(), router_names[i + 1].length());
                auto router3 = _mapping.get_data(res.second);



                current = router1->find_interface(router_name);
                inter1 = router->find_interface(router_names[i - 5]);

                current->make_pairing(inter1);

                current = router2->find_interface(router_name);
                inter2 = router->find_interface(router_names[i + 2]);

                current->make_pairing(inter2);

                current = router3->find_interface(router_name);
                inter3 = router->find_interface(router_names[i + 1]);

                current->make_pairing(inter3);


            }
                iteration_nr -= 5;
                break;
            default:
                throw base_error("Something went wrong in the construction");
        }
        //Goto next Router
    }
    Router::add_null_router(_routers, _all_interfaces, _mapping); //Last router
    //Construct Network
    return Network(std::move(_mapping), std::move(_routers), std::move(_all_interfaces));
}

BOOST_AUTO_TEST_CASE(NetworkConstruction) {
    Network synthetic_network = createmapping();
    synthetic_network.print_dot(std::cout);

    BOOST_CHECK_EQUAL(true, true);
}