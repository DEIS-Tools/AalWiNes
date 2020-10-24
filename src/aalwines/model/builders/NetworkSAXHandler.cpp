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
 * File:   NetworkSAXHandler.cpp
 * Author: Morten K. Schou <morten@h-schou.dk>
 *
 * Created on 19-10-2020.
 */

#include "NetworkSAXHandler.h"

namespace aalwines {
    std::ostream& operator<<(std::ostream& s, NetworkSAXHandler::keys key) {
        switch (key) {
            case NetworkSAXHandler::keys::none:
                s << "<initial>";
                break;
            case NetworkSAXHandler::keys::unknown:
                s << "<unknown>";
                break;
            case NetworkSAXHandler::keys::network:
                s << "network";
                break;
            case NetworkSAXHandler::keys::network_name:
                s << "name";
                break;
            case NetworkSAXHandler::keys::routers:
                s << "routers";
                break;
            case NetworkSAXHandler::keys::links:
                s << "links";
                break;
            case NetworkSAXHandler::keys::router_names:
                s << "names";
                break;
            case NetworkSAXHandler::keys::location:
                s << "location";
                break;
            case NetworkSAXHandler::keys::latitude:
                s << "latitude";
                break;
            case NetworkSAXHandler::keys::longitude:
                s << "longitude";
                break;
            case NetworkSAXHandler::keys::interfaces:
                s << "interfaces";
                break;
            case NetworkSAXHandler::keys::interface_name:
                s << "name";
                break;
            case NetworkSAXHandler::keys::routing_table:
                s << "routing_table";
                break;
            case NetworkSAXHandler::keys::table_label:
                s << "<label in routing table>";
                break;
            case NetworkSAXHandler::keys::entry_out:
                s << "out";
                break;
            case NetworkSAXHandler::keys::priority:
                s << "priority";
                break;
            case NetworkSAXHandler::keys::ops:
                s << "ops";
                break;
            case NetworkSAXHandler::keys::weight:
                s << "weight";
                break;
            case NetworkSAXHandler::keys::pop:
                s << "pop";
                break;
            case NetworkSAXHandler::keys::swap:
                s << "swap";
                break;
            case NetworkSAXHandler::keys::push:
                s << "push";
                break;
            case NetworkSAXHandler::keys::from_router:
                s << "from_router";
                break;
            case NetworkSAXHandler::keys::from_interface:
                s << "from_interface";
                break;
            case NetworkSAXHandler::keys::to_router:
                s << "to_router";
                break;
            case NetworkSAXHandler::keys::to_interface:
                s << "to_interface";
                break;
            case NetworkSAXHandler::keys::bidirectional:
                s << "bidirectional";
                break;
        }
        return s;
    }

    std::ostream &operator<<(std::ostream& s, NetworkSAXHandler::context::context_type type) {
        switch (type) {
            case NetworkSAXHandler::context::context_type::unknown:
                s << "<unknown>";
                break;
            case NetworkSAXHandler::context::context_type::initial:
                s << "initial";
                break;
            case NetworkSAXHandler::context::context_type::network:
                s << "network";
                break;
            case NetworkSAXHandler::context::context_type::link_array:
                s << "link array";
                break;
            case NetworkSAXHandler::context::context_type::link:
                s << "link";
                break;
            case NetworkSAXHandler::context::context_type::router_array:
                s << "router array";
                break;
            case NetworkSAXHandler::context::context_type::router:
                s << "router";
                break;
            case NetworkSAXHandler::context::context_type::router_name_array:
                s << "router name array";
                break;
            case NetworkSAXHandler::context::context_type::location:
                s << "location";
                break;
            case NetworkSAXHandler::context::context_type::routing_table:
                s << "routing table";
                break;
            case NetworkSAXHandler::context::context_type::interface_array:
                s << "interface array";
                break;
            case NetworkSAXHandler::context::context_type::interface:
                s << "interface";
                break;
            case NetworkSAXHandler::context::context_type::entry_array:
                s << "entry array";
                break;
            case NetworkSAXHandler::context::context_type::entry:
                s << "entry";
                break;
            case NetworkSAXHandler::context::context_type::operation_array:
                s << "operation array";
                break;
            case NetworkSAXHandler::context::context_type::operation:
                s << "operation";
                break;
        }
        return s;
    }

    bool NetworkSAXHandler::pair_link(const std::string &from_router_name, const std::string &from_interface_name,
                                      const std::string &to_router_name, const std::string &to_interface_name) {
        auto [from_exists, from_id] = router_map.exists(from_router_name);
        if(!from_exists) {
            errors << "error: No router with name \"" << from_router_name << "\" was defined." << std::endl;
            return false;
        }
        auto from_router = router_map.get_data(from_id);

        auto [to_exists, to_id] = router_map.exists(from_router_name);
        if(!to_exists) {
            errors << "error: No router with name \"" << to_router_name << "\" was defined." << std::endl;
            return false;
        }
        auto to_router = router_map.get_data(to_id);

        auto from_interface = from_router->find_interface(from_interface_name);
        auto to_interface = to_router->find_interface(to_interface_name);
        if (from_interface == nullptr) {
            errors << "error: No interface with name \"" << from_interface_name << "\" was defined for router \"" << from_router_name << "\"." << std::endl;
            return false;
        }
        if (to_interface == nullptr) {
            errors << "error: No interface with name \"" << to_interface_name << "\" was defined for router \"" << to_router_name << "\"." << std::endl;
            return false;
        }
        if ((from_interface->match() != nullptr && from_interface->match() != to_interface) || (to_interface->match() != nullptr && to_interface->match() != from_interface)) {
            errors << R"(error: Conflicting link: ["from_router":")" << from_router_name << R"(", "from_interface":")" << from_interface_name
                   << R"(", "to_router":")" << to_router_name << R"(", "to_interface":")" << to_interface_name << R"("])" << std::endl;
            return false;
        }
        from_interface->make_pairing(to_interface);
        return true;
    }

    bool NetworkSAXHandler::null() {
        switch (last_key) {
            case keys::unknown:
                break;
            default:
                errors << "error: Unexpected null value after key: " << last_key << std::endl;
                return false;
        }
        return true;
    }

    bool NetworkSAXHandler::boolean(bool value) {
        switch (last_key) {
            case keys::bidirectional:
            case keys::unknown:
                break;
            default:
                errors << "error: Unexpected boolean value: " << value << " after key: " << last_key << std::endl;
                return false;
        }
        return true;
    }

    bool NetworkSAXHandler::number_integer(NetworkSAXHandler::number_integer_t value) {
        switch (last_key) {
            case keys::latitude:
                latitude = (double)value;
                break;
            case keys::longitude:
                longitude = (double)value;
                break;
            case keys::unknown:
                break;
            default:
                errors << "error: Integer value: " << value << " found after key:" << last_key << std::endl;
                return false;
        }
        return true;
    }

    bool NetworkSAXHandler::number_unsigned(NetworkSAXHandler::number_unsigned_t value) {
        switch (last_key) {
            case keys::priority:
                priority = value;
                break;
            case keys::weight:
                weight = value;
                break;
            case keys::latitude:
                latitude = (double)value;
                break;
            case keys::longitude:
                longitude = (double)value;
                break;
            case keys::unknown:
                break;
            default:
                errors << "error: Unsigned value: " << value << " found after key: " << last_key << std::endl;
                return false;
        }
        return true;
    }

    bool NetworkSAXHandler::number_float(NetworkSAXHandler::number_float_t value, const NetworkSAXHandler::string_t &) {
        switch (last_key) {
            case keys::latitude:
                latitude = value;
                break;
            case keys::longitude:
                longitude = value;
                break;
            case keys::unknown:
                break;
            default:
                errors << "error: Float value: " << value << " comes after key: " << last_key << std::endl;
                return false;
        }
        return true;
    }

    bool NetworkSAXHandler::string(NetworkSAXHandler::string_t &value) {
        if (context_stack.empty()) {
            errors << "error: Unexpected string value: \"" << value << "\" outside of object." << std::endl;
            return false;
        }
        if (context_stack.top().type == context::context_type::router_name_array) {
            current_router->add_name(value);
            auto res = router_map.insert(value);
            if (!res.first) {
                errors << "error: Duplicate definition of \"" << value << "\", previously found in entry "
                       << router_map.get_data(res.second)->index() << std::endl;
                return false;
            }
            router_map.get_data(res.second) = current_router;
            return true;
        }
        switch (last_key){
            case keys::network_name:
                network_name = value;
                break;
            case keys::interface_name: {
                auto [was_inserted, interface] = current_router->insert_interface(value, all_interfaces);
                if (!was_inserted) {
                    // Was this added because of an out-interface in an already parsed routing-table?
                    auto f = forward_constructed_interfaces.find(value);
                    if (f != forward_constructed_interfaces.end()) {
                        forward_constructed_interfaces.erase(f); // Then update set of pre constructed interfaces.
                    } else { // Otherwise we have duplicate interface definition.
                        errors << "error: Duplicate interface name \"" << value << "\" on router \"" << current_router->name() << "\"." << std::endl;
                        return false;
                    }
                }
                current_interface = interface;
                break;
            }
            case keys::entry_out: {
                auto [was_inserted, interface] = current_router->insert_interface(value, all_interfaces);
                if (was_inserted) {
                    forward_constructed_interfaces.insert(value);
                }
                via = interface;
                break;
            }
            case keys::pop:
                assert(value.empty()); // TODO: Should this be error?
                ops.emplace_back(RoutingTable::op_t::POP, Query::label_t{});
                break;
            case keys::swap:
                ops.emplace_back(RoutingTable::op_t::SWAP, Query::label_t(value));
                break;
            case keys::push:
                ops.emplace_back(RoutingTable::op_t::PUSH, Query::label_t(value));
                break;
            case keys::from_interface:
                current_from_interface_name = value;
                break;
            case keys::from_router:
                current_from_router_name = value;
                break;
            case keys::to_interface:
                current_to_interface_name = value;
                break;
            case keys::to_router:
                current_to_router_name = value;
                break;
            case keys::unknown:
                break;
            default:
            case keys::none:
                errors << "error: String value: " << value << " found after key:" << last_key << std::endl;
                return false;
        }
        return true;
    }

    bool NetworkSAXHandler::start_object(std::size_t) {
        if (context_stack.empty()) {
            context_stack.push(initial_context);
            return true;
        }
        switch (context_stack.top().type) {
            case context::context_type::router_array:
                context_stack.push(router_context);
                routers.emplace_back(std::make_unique<Router>(routers.size()));
                current_router = routers.back().get();
                return true;
            case context::context_type::link_array:
                context_stack.push(link_context);
                return true;
            case context::context_type::interface_array:
                context_stack.push(interface_context);
                current_table = RoutingTable{};
                return true;
            case context::context_type::entry_array:
                context_stack.push(entry_context);
                weight = 0;
                return true;
            case context::context_type::operation_array:
                context_stack.push(operation_context);
                return true;
            default:
                break;
        }
        switch (last_key) {
            case keys::network:
                context_stack.push(network_context);
                break;
            case keys::location:
                context_stack.push(location_context);
                break;
            case keys::routing_table:
                context_stack.push(table_context);
                break;
            case keys::unknown:
                context_stack.push(unknown_context);
                break;
            default:
                errors << "error: Found object after key: " << last_key << std::endl;
                return false;
        }
        return true;
    }

    bool NetworkSAXHandler::key(NetworkSAXHandler::string_t &key) {
        if (context_stack.empty()) {
            errors << "Expected the start of an object before key: " << key << std::endl;
            return false;
        }
        switch (context_stack.top().type) {
            case context::context_type::initial:
                if (key == "network") {
                    if (!context_stack.top().needs_value(context::FLAG_1)) {
                        errors << R"(Duplicate definition of key: "network" in initial object)" << std::endl;
                        return false;
                    }
                    context_stack.top().got_value(context::FLAG_1);
                    last_key = keys::network;
                } else {
                    last_key = keys::unknown;
                }
                break;
            case context::context_type::network:
                if (key == "name") {
                    if (!context_stack.top().needs_value(context::FLAG_1)) {
                        errors << R"(Duplicate definition of key: "name" in network object)" << std::endl;
                        return false;
                    }
                    context_stack.top().got_value(context::FLAG_1);
                    last_key = keys::network_name;
                } else if (key == "routers") {
                    if (!context_stack.top().needs_value(context::FLAG_2)) {
                        errors << R"(Duplicate definition of key: "routers" in network object)" << std::endl;
                        return false;
                    }
                    context_stack.top().got_value(context::FLAG_2);
                    last_key = keys::routers;
                } else if (key == "links") {
                    if (!context_stack.top().needs_value(context::FLAG_3)) {
                        errors << R"(Duplicate definition of key: "links" in network object)" << std::endl;
                        return false;
                    }
                    context_stack.top().got_value(context::FLAG_3);
                    last_key = keys::links;
                } else { // "additionalProperties": true
                    last_key = keys::unknown;
                }
                break;
            case context::context_type::link:
                if (key == "from_interface") {
                    if (!context_stack.top().needs_value(context::FLAG_1)) {
                        errors << R"(Duplicate definition of key: "from_interface" in link object)" << std::endl;
                        return false;
                    }
                    context_stack.top().got_value(context::FLAG_1);
                    last_key = keys::from_interface;
                } else if (key == "from_router") {
                    if (!context_stack.top().needs_value(context::FLAG_2)) {
                        errors << R"(Duplicate definition of key: "from_router" in link object)" << std::endl;
                        return false;
                    }
                    context_stack.top().got_value(context::FLAG_2);
                    last_key = keys::from_router;
                } else if (key == "to_interface") {
                    if (!context_stack.top().needs_value(context::FLAG_3)) {
                        errors << R"(Duplicate definition of key: "to_interface" in link object)" << std::endl;
                        return false;
                    }
                    context_stack.top().got_value(context::FLAG_3);
                    last_key = keys::to_interface;
                } else if (key == "to_router") {
                    if (!context_stack.top().needs_value(context::FLAG_4)) {
                        errors << R"(Duplicate definition of key: "to_router" in link object)" << std::endl;
                        return false;
                    }
                    context_stack.top().got_value(context::FLAG_4);
                    last_key = keys::to_router;
                } else if (key == "bidirectional") {
                    last_key = keys::bidirectional;
                } else { // "additionalProperties": true
                    last_key = keys::unknown;
                }
                break;
            case context::context_type::router:
                if (key == "names") {
                    if (!context_stack.top().needs_value(context::FLAG_1)) {
                        errors << R"(Duplicate definition of key: "names" in router object)" << std::endl;
                        return false;
                    }
                    context_stack.top().got_value(context::FLAG_1);
                    last_key = keys::router_names;
                } else if (key == "interfaces") {
                    if (!context_stack.top().needs_value(context::FLAG_2)) {
                        errors << R"(Duplicate definition of key: "interfaces" in router object)" << std::endl;
                        return false;
                    }
                    context_stack.top().got_value(context::FLAG_2);
                    last_key = keys::interfaces;
                } else if (key == "location") {
                    last_key = keys::location;
                } else {
                    errors << "Unexpected key in router object: " << key << std::endl;
                    return false;
                }
                break;
            case context::context_type::interface:
                if (key == "name") {
                    if (!context_stack.top().needs_value(context::FLAG_1)) {
                        errors << R"(Duplicate definition of key: "name" in interface object)" << std::endl;
                        return false;
                    }
                    context_stack.top().got_value(context::FLAG_1);
                    last_key = keys::interface_name;
                } else if (key == "routing_table") {
                    if (!context_stack.top().needs_value(context::FLAG_2)) {
                        errors << R"(Duplicate definition of key: "routing_table" in interface object)" << std::endl;
                        return false;
                    }
                    context_stack.top().got_value(context::FLAG_2);
                    last_key = keys::routing_table;
                } else {
                    errors << "Unexpected key in router object: " << key << std::endl;
                    return false;
                }
                break;
            case context::context_type::routing_table:
                last_key = keys::table_label;
                current_table.emplace_entry(RoutingTable::label_t(key));
                break;
            case context::context_type::entry:
                if (key == "out") {
                    if (!context_stack.top().needs_value(context::FLAG_1)) {
                        errors << R"(Duplicate definition of key: "out" in routing entry object)" << std::endl;
                        return false;
                    }
                    context_stack.top().got_value(context::FLAG_1);
                    last_key = keys::entry_out;
                } else if (key == "priority") {
                    if (!context_stack.top().needs_value(context::FLAG_2)) {
                        errors << R"(Duplicate definition of key: "priority" in routing entry object)" << std::endl;
                        return false;
                    }
                    context_stack.top().got_value(context::FLAG_2);
                    last_key = keys::priority;
                } else if (key == "ops") {
                    if (!context_stack.top().needs_value(context::FLAG_3)) {
                        errors << R"(Duplicate definition of key: "ops" in routing entry object)" << std::endl;
                        return false;
                    }
                    context_stack.top().got_value(context::FLAG_3);
                    last_key = keys::ops;
                } else if (key == "weight") {
                    last_key = keys::weight;
                } else {
                    errors << "Unexpected key in table entry object: " << key << std::endl;
                    return false;
                }
                break;
            case context::context_type::operation:
                if (key == "pop") {
                    if (!context_stack.top().needs_value(context::FLAG_1)) {
                        errors << R"(Duplication key definition in operation object)" << std::endl;
                        return false;
                    }
                    context_stack.top().got_value(context::FLAG_1);
                    last_key = keys::pop;
                } else if (key == "swap") {
                    if (!context_stack.top().needs_value(context::FLAG_1)) {
                        errors << R"(Duplication key definition in operation object)" << std::endl;
                        return false;
                    }
                    context_stack.top().got_value(context::FLAG_1);
                    last_key = keys::swap;
                } else if (key == "push") {
                    if (!context_stack.top().needs_value(context::FLAG_1)) {
                        errors << R"(Duplication key definition in operation object)" << std::endl;
                        return false;
                    }
                    context_stack.top().got_value(context::FLAG_1);
                    last_key = keys::push;
                } else {
                    errors << "Unexpected key in operation object: " << key << std::endl;
                    return false;
                }
                break;
            case context::context_type::location:
                if (key == "latitude") {
                    if (!context_stack.top().needs_value(context::FLAG_1)) {
                        errors << R"(Duplicate definition of key: "latitude" in location object)" << std::endl;
                        return false;
                    }
                    context_stack.top().got_value(context::FLAG_1);
                    last_key = keys::latitude;
                } else if (key == "longitude") {
                    if (!context_stack.top().needs_value(context::FLAG_2)) {
                        errors << R"(Duplicate definition of key: "longitude" in location object)" << std::endl;
                        return false;
                    }
                    context_stack.top().got_value(context::FLAG_2);
                    last_key = keys::longitude;
                } else { // "additionalProperties": true
                    last_key = keys::unknown;
                }
                break;
            case context::context_type::unknown:
                break;
            default:
                errors << "error: Encountered unexpected key: \"" << key << "\" in context: " << context_stack.top().type << std::endl;
                return false;
        }
        return true;
    }

    bool NetworkSAXHandler::end_object() {
        if (context_stack.empty()) {
            errors << "error: Unexpected end of object." << std::endl;
            return false;
        }
        if (context_stack.top().missing_keys()) {
            errors << "error: Missing key." << std::endl; // TODO: Improve error message.
            return false;
        }
        switch (context_stack.top().type) {
            case context::context_type::link:{
                if (routers_parsed) {
                    auto success = pair_link(current_from_router_name, current_from_interface_name, current_to_router_name, current_to_interface_name);
                    if (!success) return false;
                } else {
                    links.emplace_back(current_from_router_name, current_from_interface_name, current_to_router_name, current_to_interface_name);
                }
                break;
            }
            case context::context_type::router:
                if (!forward_constructed_interfaces.empty()) {
                    errors << "error: Interface " << *forward_constructed_interfaces.begin() << " used in a routing table is not defined on router " << current_router->name() << "." << std::endl ;
                    return false;
                }
                break;
            case context::context_type::location:
                current_router->set_coordinate(Coordinate(latitude, longitude));
                break;
            case context::context_type::interface:
                current_interface->table() = std::move(current_table);
                break;
            case context::context_type::entry:
                current_table.back()._rules.emplace_back(RoutingTable::type_t::MPLS, std::move(ops), via, priority, weight);
                break;
            default:
                break;
        }
        context_stack.pop();
        return true;
    }

    bool NetworkSAXHandler::start_array(std::size_t) {
        if (context_stack.empty()) {
            errors << "error: Encountered start of array, but must start with an object." << std::endl;
            return false;
        }
        switch (last_key) {
            case keys::routers:
                context_stack.push(router_array);
                break;
            case keys::links:
                context_stack.push(link_array);
                break;
            case keys::interfaces:
                context_stack.push(interface_array);
                break;
            case keys::router_names:
                context_stack.push(router_name_array);
                break;
            case keys::table_label:
                context_stack.push(entry_array);
                break;
            case keys::ops:
                context_stack.push(operation_array);
                break;
            case keys::unknown:
                context_stack.push(unknown_context);
                break;
            default:
                errors << "Unexpected start of array after key " << last_key << std::endl;
                return false;
        }
        return true;
    }

    bool NetworkSAXHandler::end_array() {
        if (context_stack.empty()) {
            errors << "error: Unexpected end of array." << std::endl;
            return false;
        }
        switch (context_stack.top().type) {
            case context::context_type::router_array:
                routers_parsed = true;
                for (const auto &[from_router, from_interface, to_router, to_interface] : links) {
                    pair_link(from_router, from_interface, to_router, to_interface);
                }
                break;
            default:
                break;
        }
        context_stack.pop();
        return true;
    }

    bool NetworkSAXHandler::parse_error(std::size_t location, const std::string &last_token,
                                        const nlohmann::detail::exception &e) {
        errors << "error at line " << location << " with last token " << last_token << ". " << std::endl;
        errors << "\terror message: " << e.what() << std::endl;
        return false;
    }
}