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
 * File:   NetworkSAXHandler.h
 * Author: Morten K. Schou <morten@h-schou.dk>
 *
 * Created on 19-10-2020.
 */

#ifndef AALWINES_NETWORKSAXHANDLER_H
#define AALWINES_NETWORKSAXHANDLER_H

#include <json.hpp>
#include <aalwines/model/Network.h>

using json = nlohmann::json;

namespace aalwines {

    //template<typename BasicJsonType = json>
    class NetworkSAXHandler {
        struct context {
            enum class object_type : uint32_t { initial, network, link_array, link, router_array, router, interface_array, interface, routing_table, entry_array, entry, location };
            enum key_flag : uint32_t {
                NO_FLAGS = 0,
                FLAG_1 = 1,
                FLAG_2 = 2,
                FLAG_3 = 4,
                FLAG_4 = 8,
                // Required values for each object type.
                REQUIRES_0 = NO_FLAGS,
                REQUIRES_1 = FLAG_1,
                REQUIRES_2 = FLAG_1 | FLAG_2,
                REQUIRES_3 = FLAG_1 | FLAG_2 | FLAG_3,
                REQUIRES_4 = FLAG_1 | FLAG_2 | FLAG_3 | FLAG_4
            };
            object_type object;
            key_flag values_left;

            void got_value(key_flag value) {
                values_left = static_cast<context::key_flag>(static_cast<uint32_t>(values_left) & ~static_cast<uint32_t>(value));
            }

            [[nodiscard]] bool missing_keys() const {
                return values_left != NO_FLAGS;
            }

        };
        enum class keys : uint32_t { none, network, network_name, routers, links, router_names, location, latitude, longitude, interfaces, interface_name, routing_table, table_label, entry_out, priority, ops, weight, from_router, from_interface, to_router, to_interface, bidirectional };
        friend std::ostream& operator<<( std::ostream&, keys key );

        constexpr static context initial_context = { context::object_type::initial, context::REQUIRES_1 };
        constexpr static context network_context = { context::object_type::network, context::REQUIRES_3 };
        constexpr static context link_context = { context::object_type::link, context::REQUIRES_4 };
        constexpr static context router_context = { context::object_type::router, context::REQUIRES_2 };
        constexpr static context interface_context = { context::object_type::interface, context::REQUIRES_2 };
        constexpr static context location_context = { context::object_type::location, context::REQUIRES_2 };
        constexpr static context table_context = { context::object_type::routing_table, context::REQUIRES_0 };
        constexpr static context entry_context = { context::object_type::entry, context::REQUIRES_3 };

        std::stack<context> context_stack;
        context next_context = initial_context;
        keys last_key = keys::none;
        std::ostream& errors;

        string_map<Router*> router_map;
        std::vector<std::unique_ptr<Router>> routers;
        std::vector<const Interface*> all_interfaces;
        std::string network_name;

        // Router
        Router* current_router;
        double latitude = 0;
        double longitude = 0;

        // Interface
        Interface* current_interface;
        RoutingTable current_table;

        // Table
        RoutingTable::entry_t* current_entry; // current_top_label;
        // Entry
        Interface* via;
        size_t priority;
        uint32_t weight;
        std::vector<RoutingTable::action_t> ops;

        // Link
        std::string from_interface_name;
        std::string from_router_name;
        std::string to_interface_name;
        std::string to_router_name;
        // bool bidirectional = false; // Not used

    public:
        using number_integer_t = typename json::number_integer_t;
        using number_unsigned_t = typename json::number_unsigned_t;
        using number_float_t = typename json::number_float_t;
        using string_t = typename json::string_t;

        NetworkSAXHandler(std::ostream& errors = std::cerr) : errors(errors) {};

        Network get_network() {
            Network network(std::move(router_map), std::move(routers), std::move(all_interfaces));
            network.add_null_router();
            network.name = network_name;
            return network;
        }

        bool null() {
            errors << "error: Unexpected null value." << std::endl;
            return false;
        }

        bool boolean(bool value) {
            if (last_key != keys::bidirectional) {
                errors << "Unexpected bool. ";
                return false;
            }
            return true;
        }

        bool number_integer(number_integer_t value) {
            switch (last_key) {
                default:
                    errors << "error: String value: " << value << " found after key:" << last_key << std::endl;
                    return false;
            }
            return true;
        }

        bool number_unsigned(number_unsigned_t value) {
            switch (last_key) {
                case keys::priority:
                    priority = value;
                    break;
                case keys::weight:
                    weight = value;
                    break;
                default:
                    errors << "error: String value: " << value << " found after key:" << last_key << std::endl;
                    return false;
            }
            return true;
        }

        bool number_float(number_float_t value, const string_t& /*unused*/) {
            switch (last_key) {
                case keys::latitude:
                    latitude = value;
                    break;
                case keys::longitude:
                    longitude = value;
                    break;
                default:
                    errors << "Float value: " << value << " comes after unexpected key." << std::endl;
                    return false;
            }
            return true;
        }

        bool string(string_t& value) {
            switch (last_key){
                case keys::network_name:
                    network_name = value;
                    break;
                case keys::router_names: {
                    current_router->add_name(value);
                    auto res = router_map.insert(value);
                    if (!res.first) {
                        errors << "error: Duplicate definition of \"" << value << "\", previously found in entry "
                               << router_map.get_data(res.second)->index() << std::endl;
                        return false;
                    }
                    router_map.get_data(res.second) = current_router;
                    break;
                }
                case keys::interface_name: {
                    auto [was_inserted, _] = current_router->insert_interface(value, all_interfaces);
                    if (!was_inserted) {
                        errors << "error: Duplicate interface name \"" << value << "\" on router \"" << current_router->name() << "\"." << std::endl;
                        return false;
                    }
                    break;
                }
                case keys::entry_out:
                    via = current_router->find_interface(value);
                    break;
                case keys::from_interface:
                    from_interface_name = value;
                    break;
                case keys::from_router:
                    from_router_name = value;
                    break;
                case keys::to_interface:
                    to_interface_name = value;
                    break;
                case keys::to_router:
                    to_router_name = value;
                    break;
                default:
                case keys::none:
                    errors << "error: String value: " << value << " found after key:" << last_key << std::endl;
                    return false;
            }
            return true;
        }

        bool start_object(std::size_t /*unused*/ = std::size_t(-1)) {
            switch (last_key) {
                case keys::network:
                    context_stack.push(network_context);
                    break;
                case keys::routers:
                    context_stack.push(router_context);
                    routers.emplace_back(std::make_unique<Router>(routers.size()));
                    current_router = routers.back().get();
                    break;
                case keys::links:
                    context_stack.push(link_context);
                    break;
                case keys::location:
                    context_stack.push(location_context);
                    break;
                case keys::interfaces:
                    context_stack.push(interface_context);
                    current_table = RoutingTable{};
                    break;
                case keys::routing_table:
                    context_stack.push(table_context);
                    break;
                case keys::table_label:
                    context_stack.push(entry_context);
                    weight = 0;
                default:
                    errors << "error: Found object after key: " << last_key << std::endl;
                    return false;
            }
            return true;
        }

        bool key(string_t& key) {
            switch (context_stack.top().object) {
                case context::object_type::initial:
                    if (key == "network") {
                        context_stack.top().got_value(context::FLAG_1);
                        last_key = keys::network;
                    }
                    break;
                case context::object_type::network:
                    if (key == "name") {
                        context_stack.top().got_value(context::FLAG_1);
                        last_key = keys::network_name;
                    } else if (key == "routers") {
                        context_stack.top().got_value(context::FLAG_2);
                        last_key = keys::routers;
                    } else if (key == "links") {
                        context_stack.top().got_value(context::FLAG_3);
                        last_key = keys::links;
                    } // "additionalProperties": true
                    break;
                case context::object_type::link:
                    if (key == "from_interface") {
                        context_stack.top().got_value(context::FLAG_1);
                        last_key = keys::from_interface;
                    } else if (key == "from_router") {
                        context_stack.top().got_value(context::FLAG_2);
                        last_key = keys::from_router;
                    } else if (key == "to_interface") {
                        context_stack.top().got_value(context::FLAG_3);
                        last_key = keys::to_interface;
                    } else if (key == "to_router") {
                        context_stack.top().got_value(context::FLAG_4);
                        last_key = keys::to_router;
                    } else if (key == "bidirectional") {
                        last_key = keys::bidirectional;
                    } // "additionalProperties": true
                    /* else {
                        errors << "Unexpected key in link object: " << key << std::endl;
                        return false;
                    }*/
                    break;
                case context::object_type::router:
                    if (key == "names") {
                        context_stack.top().got_value(context::FLAG_1);
                        last_key = keys::router_names;
                    } else if (key == "interfaces") {
                        context_stack.top().got_value(context::FLAG_2);
                        last_key = keys::interfaces;
                    } else if (key == "location") {
                        last_key = keys::location;
                    } else {
                        errors << "Unexpected key in router object: " << key << std::endl;
                        return false;
                    }
                    break;
                case context::object_type::interface:
                    if (key == "name") {
                        context_stack.top().got_value(context::FLAG_1);
                        last_key = keys::interface_name;
                    } else if (key == "routing_table") {
                        context_stack.top().got_value(context::FLAG_2);
                        last_key = keys::routing_table;
                    } else {
                        errors << "Unexpected key in router object: " << key << std::endl;
                        return false;
                    }
                    break;
                case context::object_type::routing_table:
                    //current_top_label = RoutingTable::label_t(key);
                    last_key = keys::table_label;
                    current_table.emplace_entry(RoutingTable::label_t(key));
                    break;
                case context::object_type::entry:
                    if (key == "out") {
                        context_stack.top().got_value(context::FLAG_1);
                        last_key = keys::entry_out;
                    } else if (key == "priority") {
                        context_stack.top().got_value(context::FLAG_2);
                        last_key = keys::priority;
                    } else if (key == "ops") {
                        context_stack.top().got_value(context::FLAG_3);
                        last_key = keys::ops;
                    } else if (key == "weight") {
                        last_key = keys::weight;
                    } else {
                        errors << "Unexpected key in table entry object: " << key << std::endl;
                        return false;
                    }
                    break;
                case context::object_type::location:
                    if (key == "latitude") {
                        context_stack.top().got_value(context::FLAG_1);
                        last_key = keys::latitude;
                    } else if (key == "longitude") {
                        context_stack.top().got_value(context::FLAG_2);
                        last_key = keys::longitude;
                    }
                    break;
                default:
                    break;
            }
            return true;
        }

        bool end_object() {
            if (context_stack.top().missing_keys()) {
                errors << "Missing key." << std::endl; // TODO: Improve error message.
                return false;
            }
            switch (context_stack.top().object) {
                case context::object_type::location:
                    current_router->set_coordinate(Coordinate(latitude, longitude));
                    break;
                case context::object_type::interface:
                    current_interface->table() = std::move(current_table);
                    break;
                case context::object_type::entry:
                    current_table.back()._rules.emplace_back(RoutingTable::type_t::MPLS, std::move(ops), via, priority, weight);
                default:
                    break;
            }
            context_stack.pop();
            return true;
        }

        bool start_array(std::size_t /*unused*/ = std::size_t(-1)) {
            return true;
        }

        bool end_array() {
            return true;
        }

        bool parse_error(std::size_t /*unused*/, const std::string& /*unused*/, const nlohmann::detail::exception& /*unused*/) {
            return false;
        }
    };

    class FastJsonBuilder {
        static Network parse(std::istream& stream, std::ostream& warnings) {
            std::stringstream es; // For errors;
            NetworkSAXHandler my_sax(es);
            if (!json::sax_parse(stream, &my_sax)) {
                throw base_error(es.str());
            }
            return my_sax.get_network();
        }

        static Network parse(const std::string& network_file, std::ostream& warnings) {
            std::ifstream stream(network_file);
            if (!stream.is_open()) {
                std::stringstream es;
                es << "error: Could not open file : " << network_file << std::endl;
                throw base_error(es.str());
            }
            return parse(stream, warnings);
        }
    };

}

#endif //AALWINES_NETWORKSAXHANDLER_H
