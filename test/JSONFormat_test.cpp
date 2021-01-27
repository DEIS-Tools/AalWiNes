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
 * File:   JSONFormat_test.cpp
 * Author: Morten K. Schou <morten@h-schou.dk>
 *
 * Created on 14-08-2020.
 */

#define BOOST_TEST_MODULE JSONFormatTest

#include <boost/test/unit_test.hpp>
#include <aalwines/model/builders/AalWiNesBuilder.h>
#include <aalwines/model/builders/NetworkSAXHandler.h>

using namespace aalwines;


// Check if a json array contains a certain link
bool has_link(const json& links, std::string from_router, std::string from_interface, std::string to_router, std::string to_interface){
    for (const auto& link : links) {
        if (link["from_router"] == from_router && link["from_interface"] == from_interface && link["to_router"] == to_router && link["to_interface"] == to_interface) {
            return true;
        }
        if (link.contains("bidirectional") && link["bidirectional"].is_boolean() && link["bidirectional"].get<bool>()
                && link["from_router"] == to_router && link["from_interface"] == to_interface && link["to_router"] == from_router && link["to_interface"] == from_interface) {
            return true;
        }
    }
    return false;
}
// Check if all links in smaller are contained in larger, while taking bidirectional links into account.
bool inludes_links(const json& smaller, const json& larger) {
    for (const auto& link : smaller) {
        if (!has_link(larger, link["from_router"], link["from_interface"], link["to_router"], link["to_interface"])) {
            return false;
        }
        if (link.contains("bidirectional") && link["bidirectional"].is_boolean() && link["bidirectional"].get<bool>()) {
            if (!has_link(larger, link["to_router"], link["to_interface"], link["from_router"], link["from_interface"])) {
                return false;
            }
        }
    }
    return true;
}


BOOST_AUTO_TEST_CASE(JSON_format_IO_test) {
    std::istringstream i_stream(R"({
  "network": {
    "name": "Test Network",
    "routers": [
      {
        "name": "router 1",
        "alias": ["alternative name for router 1", "one more alias for r1"],
        "location": {"latitude": 55.02, "longitude": -16},
        "interfaces": [
          {
            "names": ["interfaceA", "interfaceC"],
            "routing_table": {}
          },
          {
            "name": "interfaceB",
            "routing_table": {
              "s10": [{"out": "interfaceA", "priority": 0, "ops":[{"swap":"s11"}], "weight": 42},
                      {"out": "interfaceA", "priority": 0, "ops":[{"swap":"s21"}]},
                      {"out": "interfaceC", "priority": 1, "ops":[{"swap":"s12"},{"push":"30"}]},
                      {"out": "interfaceB", "priority": 2, "ops":[{"pop":""}]}
              ]
            }
          }
        ]
      },
      {
        "name": "router2",
        "location": {"latitude": 0, "longitude": 9.1234567890123456789},
        "interfaces": [
          {
            "name": "interfaceA",
            "routing_table": {"100": [{"out": "interfaceB", "priority": 0, "ops":[{"swap":"200"}]}]}
          },
          {
            "name": "interfaceB",
            "routing_table": {"ip": [{"out": "interfaceA", "priority": 0, "ops":[{"push":"s1"}], "weight": 42}]}
          }
        ]
      },
      {
        "name": "r3",
        "interfaces": []
      }
    ],
    "links": [
      {"from_router": "router 1", "from_interface": "interfaceA", "to_router": "router2", "to_interface": "interfaceA"},
      {"from_router": "router2", "from_interface": "interfaceB", "to_router": "alternative name for router 1", "to_interface": "interfaceB", "bidirectional": true, "weight": 3}
    ]
  }
}
)");
    json json_network;
    i_stream >> json_network; // Parse string to json object
    auto network = json_network["network"].get<Network>(); // Parse json object to Network object

    BOOST_CHECK_EQUAL(network.name, "Test Network");
    BOOST_CHECK_EQUAL(network.routers().size(), 4); // 3 routers + 1 null-router
    auto r1 = network.routers()[0].get();
    std::vector<std::string> r1_names{"alternative name for router 1", "one more alias for r1", "router 1"};
    BOOST_CHECK_EQUAL_COLLECTIONS(r1->names().begin(), r1->names().end(), r1_names.begin(), r1_names.end());

    auto r2 = network.routers()[1].get();
    BOOST_CHECK(r2->coordinate().has_value());
    BOOST_CHECK_EQUAL(r2->coordinate()->latitude(), 0);
    BOOST_CHECK_EQUAL(r2->coordinate()->longitude(), 9.1234567890123456789);

    // TODO: Check that the rest is parsed correctly...
    // Nah, if we can output the same info as we input, we probably parsed it correctly (trusting that string->json works).

    json output_network = network; // Convert Network object to json.
    BOOST_CHECK_EQUAL(output_network["name"], "Test Network");
    //BOOST_CHECK_EQUAL(output_network["routers"], json_network["network"]["routers"]); // TODO: Enable this, when we support shared routing tables internally too.

    // Due to the bidirectional flag and possible reordering, we cannot just check equality of the json arrays.
    // There is probably a faster algorithm for this, but it will do for now.
    BOOST_CHECK(inludes_links(output_network["link"], json_network["network"]["link"]));
    BOOST_CHECK(inludes_links(json_network["network"]["link"], output_network["link"]));
}


BOOST_AUTO_TEST_CASE(Fast_JSON_Parser_test) {
    std::string input_string = R"({
  "network": {
    "name": "Test Network",
    "routers": [
      {
        "names": ["router 1", "alternative name for router 1"],
        "location": {"latitude": 55.02, "longitude": -16},
        "interfaces": [
          {
            "name": "interfaceA",
            "routing_table": {}
          },
          {
            "name": "interfaceB",
            "routing_table": {
              "s10": [{"out": "interfaceA", "priority": 0, "ops":[{"swap":"s11"}], "weight": 42},
                      {"out": "interfaceA", "priority": 0, "ops":[{"swap":"s21"}]},
                      {"out": "interfaceC", "priority": 1, "ops":[{"swap":"s12"},{"push":"30"}]},
                      {"out": "interfaceB", "priority": 2, "ops":[{"pop":""}]}
              ]
            }
          },
          {
            "name": "interfaceC",
            "routing_table": {}
          }
        ]
      },
      {
        "names": ["router2"],
        "location": {"latitude": 0, "longitude": 9.1234567890123456789},
        "interfaces": [
          {
            "name": "interfaceA",
            "routing_table": {"100": [{"out": "interfaceB", "priority": 0, "ops":[{"swap":"200"}]}]}
          },
          {
            "name": "interfaceB",
            "routing_table": {"ip": [{"out": "interfaceA", "priority": 0, "ops":[{"push":"s1"}], "weight": 42}]}
          }
        ]
      },
      {
        "names": ["r3"],
        "interfaces": []
      }
    ],
    "links": [
      {"from_router": "router 1", "from_interface": "interfaceA", "to_router": "router2", "to_interface": "interfaceA"},
      {"from_router": "router2", "from_interface": "interfaceB", "to_router": "alternative name for router 1", "to_interface": "interfaceB", "bidirectional": true}
    ]
  }
}
)";
    std::istringstream i_stream(input_string);
    json json_network;
    i_stream >> json_network; // Parse string to json object
    auto normal_network = json_network["network"].get<Network>(); // Parse json object to Network object

    std::istringstream new_i_stream(input_string);
    auto network = FastJsonBuilder::parse(new_i_stream, std::cerr);

    BOOST_CHECK_EQUAL(network.name, "Test Network");
    BOOST_CHECK_EQUAL(network.routers().size(), 4); // 3 routers + 1 null-router
    auto r1 = network.routers()[0].get();
    std::vector<std::string> r1_names{"router 1", "alternative name for router 1"};
    BOOST_CHECK_EQUAL_COLLECTIONS(r1->names().begin(), r1->names().end(), r1_names.begin(), r1_names.end());

    auto r2 = network.routers()[1].get();
    BOOST_CHECK(r2->coordinate().has_value());
    BOOST_CHECK_EQUAL(r2->coordinate()->latitude(), 0);
    BOOST_CHECK_EQUAL(r2->coordinate()->longitude(), 9.1234567890123456789);

    // TODO: Check that the rest is parsed correctly...
    // Nah, if we can output the same info as we input, we probably parsed it correctly (trusting that string->json works).

    json output_network = network; // Convert Network object to json.
    BOOST_CHECK_EQUAL(output_network["name"], "Test Network");
    BOOST_CHECK_EQUAL(output_network["routers"], json_network["network"]["routers"]);
    // Due to the bidirectional flag and possible reordering, we cannot just check equality of the json arrays.
    // There is probably a faster algorithm for this, but it will do for now.
    BOOST_CHECK(inludes_links(output_network["link"], json_network["network"]["link"]));
    BOOST_CHECK(inludes_links(json_network["network"]["link"], output_network["link"]));
}