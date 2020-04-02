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

#define BOOST_TEST_MODULE FastRerouting

#include <boost/test/unit_test.hpp>
#include <aalwines/synthesis/FastRerouting.h>
#include <iostream>

using namespace aalwines;

BOOST_AUTO_TEST_CASE(FastRerouteTest) {
    std::vector<std::string> names{"Router1", "Router2", "Router3", "Router4", "Router5", "Router6"};
    std::vector<std::vector<std::string>> links{{"iRouter1", "Router2"},
                                                {"Router1", "Router3", "Router5"},
                                                {"Router2", "Router4"},
                                                {"Router3", "Router5"},
                                                {"Router2", "Router4", "Router6"},
                                                {"Router5", "iRouter6"}};
    std::vector<std::unique_ptr<Router>> routers;
    std::vector<const Interface*> interfaces;
    Network::routermap_t mapping;
    for (size_t i = 0; i < names.size(); ++i) {
        auto name = names[i];
        size_t id = routers.size();
        routers.emplace_back(std::make_unique<Router>(id));
        Router& router = *routers.back().get();
        router.add_name(name);
        auto res = mapping.insert(name.c_str(), name.length());
        assert(res.first);
        mapping.get_data(res.second) = &router;
        for (const auto& other : links[i]) {
            router.get_interface(interfaces, other);
        }
    }
    for (size_t i = 0; i < names.size(); ++i) {
        auto name = names[i];
        for (const auto &other : links[i]) {
            auto res1 = mapping.exists(name.c_str(), name.length());
            assert(res1.first);
            auto res2 = mapping.exists(other.c_str(), other.length());
            if(!res2.first) continue;
            mapping.get_data(res1.second)->find_interface(other)->make_pairing(mapping.get_data(res2.second)->find_interface(name));
        }
    }
    Router::add_null_router(routers, interfaces, mapping);

    Network network(std::move(mapping), std::move(routers), std::move(interfaces));

    auto interface = network.get_router(1)->find_interface(names[4]);
    network.get_router(0)->find_interface(names[1])->match()->table().add_rule(
            {Query::type_t::MPLS, 0, 1},
            {RoutingTable::op_t::SWAP, {Query::type_t::MPLS, 0, 2}},
            interface);
    network.get_router(1)->find_interface(names[4])->match()->table().add_rule(
            {Query::type_t::MPLS, 0, 2},
            {RoutingTable::op_t::SWAP, {Query::type_t::MPLS, 0, 3}},
            network.get_router(4)->find_interface(names[5]));

    Query::label_t max_label(Query::type_t::MPLS, 0, 42);

    BOOST_TEST_MESSAGE("Before: ");
    std::stringstream s_before;
    network.print_simple(s_before);
    BOOST_TEST_MESSAGE(s_before.str());

    auto success = FastRerouting::make_reroute(network, interface, max_label);

    BOOST_CHECK_EQUAL(success, true);

    BOOST_TEST_MESSAGE("After: ");
    std::stringstream s_after;
    network.print_simple(s_after);
    BOOST_TEST_MESSAGE(s_after.str());
}