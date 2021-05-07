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
 * File:   Network_test.cpp
 * Author: Morten K. Schou <morten@h-schou.dk>
 *
 * Created on 13-08-2020
 */

#define BOOST_TEST_MODULE NetworkTest

#include <boost/test/unit_test.hpp>
#include <aalwines/model/Network.h>


using namespace aalwines;

BOOST_AUTO_TEST_CASE(NetworkCopy) {
    Network network("Testnet");
    auto router1 = network.add_router("router1");
    auto router2 = network.add_router("router2");
    auto i0 = network.insert_interface_to("i0", router1).second;
    auto i1 = network.insert_interface_to("i1", router1).second;
    auto i2 = network.insert_interface_to("i2", router2).second;
    auto i3 = network.insert_interface_to("i3", router2).second;
    i1->make_pairing(i2);
    i0->table()->add_rule(RoutingTable::label_t("s10"), RoutingTable::action_t(RoutingTable::op_t::SWAP, RoutingTable::label_t("s11")), i1);
    i1->table()->add_rule(RoutingTable::label_t("s21"), RoutingTable::action_t(RoutingTable::op_t::SWAP, RoutingTable::label_t("s22")), i0);
    i2->table()->add_rule(RoutingTable::label_t("s11"), RoutingTable::action_t(RoutingTable::op_t::SWAP, RoutingTable::label_t("s12")), i3);
    i3->table()->add_rule(RoutingTable::label_t("s20"), RoutingTable::action_t(RoutingTable::op_t::SWAP, RoutingTable::label_t("s21")), i2);

    Network new_network(network); // Do copy.

    BOOST_CHECK_EQUAL(new_network.name, "Testnet");

    auto new_router1 = new_network.find_router("router1");
    auto new_router2 = new_network.find_router("router2");
    BOOST_CHECK_NE(new_router1, nullptr);
    BOOST_CHECK_NE(new_router2, nullptr);
    BOOST_CHECK_NE(router1, new_router1); // Pointers should be different.
    BOOST_CHECK_NE(router2, new_router2); // Pointers should be different.
    BOOST_CHECK_EQUAL(router1->name(), new_router1->name());
    BOOST_CHECK_EQUAL(router2->name(), new_router2->name());

    auto new_i0 = new_router1->find_interface("i0");
    auto new_i1 = new_router1->find_interface("i1");
    auto new_i2 = new_router2->find_interface("i2");
    auto new_i3 = new_router2->find_interface("i3");
    BOOST_CHECK_NE(new_i0, nullptr);
    BOOST_CHECK_NE(new_i1, nullptr);
    BOOST_CHECK_NE(new_i2, nullptr);
    BOOST_CHECK_NE(new_i3, nullptr);
    BOOST_CHECK_NE(i0, new_i0); // Pointers should be different.
    BOOST_CHECK_NE(i1, new_i1); // Pointers should be different.
    BOOST_CHECK_NE(i2, new_i2); // Pointers should be different.
    BOOST_CHECK_NE(i3, new_i3); // Pointers should be different.
    BOOST_CHECK_EQUAL(i0->get_name(), new_i0->get_name());
    BOOST_CHECK_EQUAL(i1->get_name(), new_i1->get_name());
    BOOST_CHECK_EQUAL(i2->get_name(), new_i2->get_name());
    BOOST_CHECK_EQUAL(i3->get_name(), new_i3->get_name());

    // Are router tables copied correctly
    BOOST_CHECK_EQUAL_COLLECTIONS(i0->table()->entries().begin(), i0->table()->entries().end(), new_i0->table()->entries().begin(), new_i0->table()->entries().end());
    BOOST_CHECK_EQUAL_COLLECTIONS(i1->table()->entries().begin(), i1->table()->entries().end(), new_i1->table()->entries().begin(), new_i1->table()->entries().end());
    BOOST_CHECK_EQUAL_COLLECTIONS(i2->table()->entries().begin(), i2->table()->entries().end(), new_i2->table()->entries().begin(), new_i2->table()->entries().end());
    BOOST_CHECK_EQUAL_COLLECTIONS(i3->table()->entries().begin(), i3->table()->entries().end(), new_i3->table()->entries().begin(), new_i3->table()->entries().end());

    BOOST_CHECK_EQUAL(new_i0->table()->entries()[0]._rules[0]._via, new_i1);
    BOOST_CHECK_EQUAL(new_i1->table()->entries()[0]._rules[0]._via, new_i0);
    BOOST_CHECK_EQUAL(new_i2->table()->entries()[0]._rules[0]._via, new_i3);
    BOOST_CHECK_EQUAL(new_i3->table()->entries()[0]._rules[0]._via, new_i2);

    // Check pairings of interfaces.
    BOOST_CHECK_EQUAL(new_i0->source(), new_router1);
    BOOST_CHECK_EQUAL(new_i0->target(), nullptr);
    BOOST_CHECK_EQUAL(new_i0->match(), nullptr);
    BOOST_CHECK_EQUAL(new_i1->source(), new_router1);
    BOOST_CHECK_EQUAL(new_i1->target(), new_router2);
    BOOST_CHECK_EQUAL(new_i1->match(), new_i2);
    BOOST_CHECK_EQUAL(new_i2->source(), new_router2);
    BOOST_CHECK_EQUAL(new_i2->target(), new_router1);
    BOOST_CHECK_EQUAL(new_i2->match(), new_i1);
    BOOST_CHECK_EQUAL(new_i3->source(), new_router2);
    BOOST_CHECK_EQUAL(new_i3->target(), nullptr);
    BOOST_CHECK_EQUAL(new_i3->match(), nullptr);
}

