/*
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

/*
 *  Copyright Morten K. Schou
 */

/*
 * File:   FastRerouting_test.cpp
 * Author: Morten K. Schou <morten@h-schou.dk>
 *
 * Created on 01-04-2020
 */

#define BOOST_TEST_MODULE RouteConstruction

#include <boost/test/unit_test.hpp>
#include <aalwines/synthesis/RouteConstruction.h>
#include <iostream>

using namespace aalwines;

BOOST_AUTO_TEST_CASE(FastRerouteTest) {
    std::vector<std::string> names{"Router1", "Router2", "Router3", "Router4", "Router5", "Router6"};
    std::vector<std::vector<std::string>> links{{"Router2"},
                                                {"Router1", "Router3", "Router5"},
                                                {"Router2", "Router4"},
                                                {"Router3", "Router5"},
                                                {"Router2", "Router4", "Router6"},
                                                {"Router5"}};
    auto network = Network::make_network(names, links);

    auto interface = network.get_router(1)->find_interface(names[4]);
    network.get_router(0)->find_interface(names[1])->match()->table().add_rule(
            1,
            {RoutingTable::op_t::SWAP, 2},
            interface);
    network.get_router(1)->find_interface(names[4])->match()->table().add_rule(
            2,
            {RoutingTable::op_t::SWAP, 3},
            network.get_router(4)->find_interface(names[5]));

    uint64_t i = 42;
    auto next_label = [&i](){return i++;};

    BOOST_TEST_MESSAGE("Before: ");
    std::stringstream s_before;
    network.print_simple(s_before);
    BOOST_TEST_MESSAGE(s_before.str());

    auto success = RouteConstruction::make_reroute(interface, next_label);

    BOOST_CHECK_EQUAL(success, true);

    BOOST_TEST_MESSAGE("After: ");
    std::stringstream s_after;
    network.print_simple(s_after);
    BOOST_TEST_MESSAGE(s_after.str());
}

BOOST_AUTO_TEST_CASE(FastRerouteWithDataFlowTest) {
    std::vector<std::string> names{"Router1", "Router2", "Router3", "Router4", "Router5", "Router6"};
    std::vector<std::vector<std::string>> links{{"Router2"},
                                                {"Router1", "Router3", "Router5"},
                                                {"Router2", "Router4"},
                                                {"Router3", "Router5"},
                                                {"Router2", "Router4", "Router6"},
                                                {"Router5"}};
    auto network = Network::make_network(names, links);

    std::vector<const Router*> path {network.get_router(0),
                                     network.get_router(1),
                                     network.get_router(4),
                                     network.get_router(5)};
    auto fail_interface = network.get_router(1)->find_interface(names[4]);
    uint64_t i = 42;
    auto next_label = [&i](){return i++;};

    BOOST_TEST_MESSAGE("Before: ");
    std::stringstream s_before;
    network.print_simple(s_before);
    BOOST_TEST_MESSAGE(s_before.str());

    auto success1 = RouteConstruction::make_data_flow(
            network.get_router(0)->find_interface("iRouter1"),
            network.get_router(5)->find_interface("iRouter6"),
            next_label, path);

    BOOST_CHECK_EQUAL(success1, true);

    BOOST_TEST_MESSAGE("After data flow: ");
    std::stringstream s_middle;
    network.print_simple(s_middle);
    BOOST_TEST_MESSAGE(s_middle.str());

    auto success2 = RouteConstruction::make_reroute(fail_interface, next_label);

    BOOST_CHECK_EQUAL(success2, true);

    BOOST_TEST_MESSAGE("After re-routing: ");
    std::stringstream s_after;
    network.print_simple(s_after);
    BOOST_TEST_MESSAGE(s_after.str());
}


BOOST_AUTO_TEST_CASE(DataFlowTest) {
    std::vector<std::string> names{"Router1", "Router2", "Router3", "Router4", "Router5", "Router6"};
    std::vector<std::vector<std::string>> links{{"Router2"},
                                                {"Router1", "Router3", "Router5"},
                                                {"Router2", "Router4"},
                                                {"Router3", "Router5"},
                                                {"Router2", "Router4", "Router6"},
                                                {"Router5"}};
    auto network = Network::make_network(names, links);

    std::vector<const Router*> path {network.get_router(0),
                                     network.get_router(1),
                                     network.get_router(4),
                                     network.get_router(5)};

    BOOST_TEST_MESSAGE("Before: ");
    std::stringstream s_before;
    network.print_simple(s_before);
    BOOST_TEST_MESSAGE(s_before.str());

    uint64_t i = 100;
    auto success = RouteConstruction::make_data_flow(
            network.get_router(0)->find_interface("iRouter1"),
            network.get_router(5)->find_interface("iRouter6"),
            [&i](){return i++;}, path);

    BOOST_CHECK_EQUAL(success, true);

    BOOST_TEST_MESSAGE("After: ");
    std::stringstream s_after;
    network.print_simple(s_after);
    BOOST_TEST_MESSAGE(s_after.str());
}

BOOST_AUTO_TEST_CASE(DataFlowWithDijkstraTest) {
    std::vector<std::string> names{"Router1", "Router2", "Router3", "Router4", "Router5", "Router6"};
    std::vector<std::vector<std::string>> links{{"Router2"},
                                                {"Router1", "Router3", "Router5"},
                                                {"Router2", "Router4"},
                                                {"Router3", "Router5"},
                                                {"Router2", "Router4", "Router6"},
                                                {"Router5"}};
    auto network = Network::make_network(names, links);

    BOOST_TEST_MESSAGE("Before: ");
    std::stringstream s_before;
    network.print_simple(s_before);
    BOOST_TEST_MESSAGE(s_before.str());

    uint64_t i = 100;
    auto success = RouteConstruction::make_data_flow(
            network.get_router(0)->find_interface("iRouter1"),
            network.get_router(5)->find_interface("iRouter6"),
            [&i](){return i++;});

    BOOST_CHECK_EQUAL(success, true);

    BOOST_TEST_MESSAGE("After: ");
    std::stringstream s_after;
    network.print_simple(s_after);
    BOOST_TEST_MESSAGE(s_after.str());
}

BOOST_AUTO_TEST_CASE(ShortDataFlowTest) {
    std::vector<std::string> names{"Router1", "Router2"};
    std::vector<std::vector<std::string>> links{{"Router2"},
                                                {"Router1"}};
    auto network = Network::make_network(names, links);

    std::vector<const Router*> path {network.get_router(0), network.get_router(1)};

    BOOST_TEST_MESSAGE("Before: ");
    std::stringstream s_before;
    network.print_simple(s_before);
    BOOST_TEST_MESSAGE(s_before.str());

    uint64_t i = 100;
    auto success = RouteConstruction::make_data_flow(
            network.get_router(0)->find_interface("iRouter1"),
            network.get_router(1)->find_interface("iRouter2"),
            [&i](){return i++;}, path);

    BOOST_CHECK_EQUAL(success, true);

    BOOST_TEST_MESSAGE("After: ");
    std::stringstream s_after;
    network.print_simple(s_after);
    BOOST_TEST_MESSAGE(s_after.str());
}

BOOST_AUTO_TEST_CASE(ShortestDataFlowTest) {
    std::vector<std::string> names{"Router1"};
    std::vector<std::vector<std::string>> links{{"iRouter1", "oRouter1"}};
    auto network = Network::make_network(names, links);

    std::vector<const Router*> path {network.get_router(0)};

    BOOST_TEST_MESSAGE("Before: ");
    std::stringstream s_before;
    network.print_simple(s_before);
    BOOST_TEST_MESSAGE(s_before.str());

    uint64_t i = 100;
    auto success = RouteConstruction::make_data_flow(
            network.get_router(0)->find_interface("iRouter1"),
            network.get_router(0)->find_interface("oRouter1"),
            [&i](){return i++;}, path);

    BOOST_CHECK_EQUAL(success, true);

    BOOST_TEST_MESSAGE("After: ");
    std::stringstream s_after;
    network.print_simple(s_after);
    BOOST_TEST_MESSAGE(s_after.str());
}

BOOST_AUTO_TEST_CASE(ShortestDataFlowWithDijkstraTest) {
    std::vector<std::string> names{"Router1"};
    std::vector<std::vector<std::string>> links{{"iRouter1", "oRouter1"}};
    auto network = Network::make_network(names, links);

    BOOST_TEST_MESSAGE("Before: ");
    std::stringstream s_before;
    network.print_simple(s_before);
    BOOST_TEST_MESSAGE(s_before.str());

    uint64_t i = 100;
    auto success = RouteConstruction::make_data_flow(
            network.get_router(0)->find_interface("iRouter1"),
            network.get_router(0)->find_interface("oRouter1"),
            [&i](){return i++;});

    BOOST_CHECK_EQUAL(success, true);

    BOOST_TEST_MESSAGE("After: ");
    std::stringstream s_after;
    network.print_simple(s_after);
    BOOST_TEST_MESSAGE(s_after.str());
}
