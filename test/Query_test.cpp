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
 * File:   Query_test.cpp
 * Author: Morten K. Schou <morten@h-schou.dk>
 *
 * Created on 10-04-2020
 */

#define BOOST_TEST_MODULE QueryTest

#include <boost/test/unit_test.hpp>
#include <aalwines/model/Network.h>
#include <aalwines/Verifier.h>
#include <aalwines/synthesis/RouteConstruction.h>

using namespace aalwines;

BOOST_AUTO_TEST_CASE(QueryTest1) {
    std::vector<std::string> routers{"Router0", "Router1"};
    std::vector<std::vector<std::string>> links{{"Router1"},{"Router0"}};

    auto network = Network::make_network(routers, links);
    uint64_t i = 42;
    auto next_label = [&i](){return i++;};
    RouteConstruction::make_data_flow(network.get_router(0)->find_interface("iRouter0"), network.get_router(1)->find_interface("iRouter1"), next_label);

    Builder builder(network);
    std::string query("<.> [.#Router0] [Router0#Router1] [Router1#.] <.> 0 OVER");

    std::istringstream qstream(query);
    builder.do_parse(qstream);

    Verifier verifier;
    verifier.set_print_trace();
    for (auto& q : builder._result) {
        auto output = verifier.run_once(builder, q);
        auto result = output["result"].get<utils::outcome_t>();
        BOOST_CHECK_EQUAL(result, utils::outcome_t::YES);
        BOOST_TEST_MESSAGE(output["trace"]);
    }
}

BOOST_AUTO_TEST_CASE(QueryTest2) {
    std::vector<std::string> routers{"Router0", "Router1"};
    std::vector<std::vector<std::string>> links{{"Router1"},{"Router0"}};

    auto network = Network::make_network(routers, links);
    uint64_t i = 42;
    auto next_label = [&i](){return i++;};
    RouteConstruction::make_data_flow(network.get_router(0)->find_interface("iRouter0"), network.get_router(1)->find_interface("iRouter1"), next_label);

    Builder builder(network);
    std::string query("<42 .> [.#Router0] [Router0#Router1] [Router1#.] <44 .> 0 OVER");

    std::istringstream qstream(query);
    builder.do_parse(qstream);

    Verifier verifier;
    verifier.set_print_trace();
    for (auto& q : builder._result) {
        auto output = verifier.run_once(builder, q);
        auto result = output["result"].get<utils::outcome_t>();
        BOOST_CHECK_EQUAL(result, utils::outcome_t::YES);
        BOOST_TEST_MESSAGE(output["trace"]);
    }
}