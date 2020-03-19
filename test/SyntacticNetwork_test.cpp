//
// Created by Morten on 18-03-2020.
//

#define BOOST_TEST_MODULE SyntacticNetwork

#include <boost/test/unit_test.hpp>
#include <aalwines/model/Network.h>

using namespace aalwines;


Network createmapping(int router_size = 5) {
    int iterations = router_size % 5;
    router_size = (iterations == 0 ? 5 : iterations * 5);
    std::string router_name = "Router";
    std::vector<std::string> router_names;
    using routermap_t = ptrie::map<char, Router *>;
    routermap_t _mapping;
    std::vector<std::unique_ptr<Router>> _routers;
    std::vector<const Interface *> _all_interfaces;

    //Synthetic Network Design.
    //Routers
    for(int i = 0; i <= router_size; i++){
        router_names.push_back(router_name + std::to_string(i));
    }

    //Build routers -> topo.xml
    //foreach router:
    for (int i = 0; i <= router_size; i++) {
        std::string name = router_name + std::to_string(i);
        router_name = router_names[i];
        _routers.emplace_back(std::make_unique<Router>(i));
        Router &router = *_routers.back().get();
        router.add_name(router_name);
        auto res = _mapping.insert(router_name.c_str(), router_name.length());
        _mapping.get_data(res.second) = &router;

        //Interfaces
        Interface *current = router.get_interface(_all_interfaces, "i" + router_name); // Will add the corresponding interface for the router
        Interface *inter1 = nullptr;
        Interface *inter2 = nullptr;
        Interface *inter3 = nullptr;

        //Routing table
        RoutingTable table;
        int interface_label;
        size_t weight = 0;
        RoutingTable::action_t op;
        RoutingTable::entry_t entry;
        Query::type_t type;

        int iteration_nr = i * iterations;
        switch (iteration_nr) {
            case 0:
                inter1 = router.get_interface(_all_interfaces, router_names[i + 1]);
                inter2 = router.get_interface(_all_interfaces, router_names[i + 2]);
                current->make_pairing(inter1);
                current->make_pairing(inter2);

                // Routing Table
                //Destinations
                entry = table.push_entry();
                interface_label = 4;
                entry._ingoing = current;           //From interface
                type = Query::MPLS;
                entry._top_label.set_value(type, interface_label, 0);

                entry._rules.emplace_back();
                entry._rules.back()._via = inter1;  //Rule to
                entry._rules.back()._type = RoutingTable::MPLS;
                entry._rules.back()._weight = weight;
                entry._rules.back()._ops.emplace_back();
                op = entry._rules.back()._ops.back();
                op._op = RoutingTable::SWAP;

                //Next Destination
                entry = table.push_entry();
                interface_label = 4;
                entry._ingoing = current;           //From interface
                type = Query::MPLS;
                entry._top_label.set_value(type, interface_label, 0);

                entry._rules.emplace_back();
                entry._rules.back()._via = inter2;  //Rule to
                entry._rules.back()._type = RoutingTable::MPLS;
                entry._rules.back()._weight = weight;
                entry._rules.back()._ops.emplace_back();
                op = entry._rules.back()._ops.back();
                op._op = RoutingTable::SWAP;
                ++weight;
                break;
            case 1:
                inter1 = router.get_interface(_all_interfaces, router_names[i - 1]);
                inter2 = router.get_interface(_all_interfaces, router_names[i + 3]);
                current->make_pairing(inter1);
                current->make_pairing(inter2);
                // Routing Table
                //Destinations
                entry = table.push_entry();
                interface_label = 4;
                entry._ingoing = current;           //From interface
                type = Query::MPLS;
                entry._top_label.set_value(type, interface_label, 0);

                entry._rules.emplace_back();
                entry._rules.back()._via = inter1;  //Rule to
                entry._rules.back()._type = RoutingTable::MPLS;
                entry._rules.back()._weight = weight;
                entry._rules.back()._ops.emplace_back();
                op = entry._rules.back()._ops.back();
                op._op = RoutingTable::SWAP;

                //Next Destination
                entry = table.push_entry();
                interface_label = 4;
                entry._ingoing = current;           //From interface
                type = Query::MPLS;
                entry._top_label.set_value(type, interface_label, 0);

                entry._rules.emplace_back();
                entry._rules.back()._via = inter2;  //Rule to
                entry._rules.back()._type = RoutingTable::MPLS;
                entry._rules.back()._weight = weight;
                entry._rules.back()._ops.emplace_back();
                op = entry._rules.back()._ops.back();
                op._op = RoutingTable::SWAP;
                ++weight;
                break;
            case 2:
                inter1 = router.get_interface(_all_interfaces, router_names[i - 2]);
                inter2 = router.get_interface(_all_interfaces, router_names[i + 1]);
                inter3 = router.get_interface(_all_interfaces, router_names[i + 2]);
                current->make_pairing(inter1);
                current->make_pairing(inter2);
                current->make_pairing(inter3);
                // Routing Table
                //Destinations
                entry = table.push_entry();
                interface_label = 4;
                entry._ingoing = current;           //From interface
                type = Query::MPLS;
                entry._top_label.set_value(type, interface_label, 0);

                entry._rules.emplace_back();
                entry._rules.back()._via = inter1;  //Rule to
                entry._rules.back()._type = RoutingTable::MPLS;
                entry._rules.back()._weight = weight;
                entry._rules.back()._ops.emplace_back();
                op = entry._rules.back()._ops.back();
                op._op = RoutingTable::SWAP;

                //Next Destination
                entry = table.push_entry();
                interface_label = 4;
                entry._ingoing = current;           //From interface
                type = Query::MPLS;
                entry._top_label.set_value(type, interface_label, 0);

                entry._rules.emplace_back();
                entry._rules.back()._via = inter2;  //Rule to
                entry._rules.back()._type = RoutingTable::MPLS;
                entry._rules.back()._weight = weight;
                entry._rules.back()._ops.emplace_back();
                op = entry._rules.back()._ops.back();
                op._op = RoutingTable::SWAP;

                //Next Destination
                entry = table.push_entry();
                interface_label = 4;
                entry._ingoing = current;           //From interface
                type = Query::MPLS;
                entry._top_label.set_value(type, interface_label, 0);

                entry._rules.emplace_back();
                entry._rules.back()._via = inter3;  //Rule to
                entry._rules.back()._type = RoutingTable::MPLS;
                entry._rules.back()._weight = weight;
                entry._rules.back()._ops.emplace_back();
                op = entry._rules.back()._ops.back();
                op._op = RoutingTable::SWAP;
                ++weight;
                break;
            case 3:
                inter1 = router.get_interface(_all_interfaces, router_names[i - 2]);
                inter2 = router.get_interface(_all_interfaces, router_names[i + 1]);
                current->make_pairing(inter1);
                current->make_pairing(inter2);
                // Routing Table
                //Destinations
                entry = table.push_entry();
                interface_label = 4;
                entry._ingoing = current;           //From interface
                type = Query::MPLS;
                entry._top_label.set_value(type, interface_label, 0);

                entry._rules.emplace_back();
                entry._rules.back()._via = inter1;  //Rule to
                entry._rules.back()._type = RoutingTable::MPLS;
                entry._rules.back()._weight = weight;
                entry._rules.back()._ops.emplace_back();
                op = entry._rules.back()._ops.back();
                op._op = RoutingTable::SWAP;

                //Next Destination
                entry = table.push_entry();
                interface_label = 4;
                entry._ingoing = current;           //From interface
                type = Query::MPLS;
                entry._top_label.set_value(type, interface_label, 0);

                entry._rules.emplace_back();
                entry._rules.back()._via = inter2;  //Rule to
                entry._rules.back()._type = RoutingTable::MPLS;
                entry._rules.back()._weight = weight;
                entry._rules.back()._ops.emplace_back();
                op = entry._rules.back()._ops.back();
                op._op = RoutingTable::SWAP;
                ++weight;
                break;
            case 4:
                inter1 = router.get_interface(_all_interfaces, router_names[i - 3]);
                inter2 = router.get_interface(_all_interfaces, router_names[i - 1]);
                inter3 = router.get_interface(_all_interfaces, router_names[i - 2]);
                current->make_pairing(inter1);
                current->make_pairing(inter2);
                current->make_pairing(inter3);
                // Routing Table
                //Destinations
                entry = table.push_entry();
                interface_label = 4;
                entry._ingoing = current;           //From interface
                type = Query::MPLS;
                entry._top_label.set_value(type, interface_label, 0);

                entry._rules.emplace_back();
                entry._rules.back()._via = inter1;  //Rule to
                entry._rules.back()._type = RoutingTable::MPLS;
                entry._rules.back()._weight = weight;
                entry._rules.back()._ops.emplace_back();
                op = entry._rules.back()._ops.back();
                op._op = RoutingTable::SWAP;

                //Next Destination
                entry = table.push_entry();
                interface_label = 4;
                entry._ingoing = current;           //From interface
                type = Query::MPLS;
                entry._top_label.set_value(type, interface_label, 0);

                entry._rules.emplace_back();
                entry._rules.back()._via = inter2;  //Rule to
                entry._rules.back()._type = RoutingTable::MPLS;
                entry._rules.back()._weight = weight;
                entry._rules.back()._ops.emplace_back();
                op = entry._rules.back()._ops.back();
                op._op = RoutingTable::SWAP;

                //Next Destination
                entry = table.push_entry();
                interface_label = 4;
                entry._ingoing = current;           //From interface
                type = Query::MPLS;
                entry._top_label.set_value(type, interface_label, 0);

                entry._rules.emplace_back();
                entry._rules.back()._via = inter3;  //Rule to
                entry._rules.back()._type = RoutingTable::MPLS;
                entry._rules.back()._weight = weight;
                entry._rules.back()._ops.emplace_back();
                op = entry._rules.back()._ops.back();
                op._op = RoutingTable::SWAP;
                ++weight;
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
    Network synthetic_network = createmapping(5);

    BOOST_CHECK_EQUAL(true, true);
}