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
    constexpr std::ostream& operator<<(std::ostream& s, NetworkSAXHandler::keys key) {
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
            case NetworkSAXHandler::keys::router_name:
                s << "name";
                break;
            case NetworkSAXHandler::keys::router_alias:
                s << "alias";
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
            case NetworkSAXHandler::keys::interface_names:
                s << "names";
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
            case NetworkSAXHandler::keys::link_weight:
                s << "weight";
                break;
        }
        return s;
    }

    constexpr std::ostream &operator<<(std::ostream& s, NetworkSAXHandler::context::context_type type) {
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
            case NetworkSAXHandler::context::context_type::router_alias_array:
                s << "router alias array";
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
            case NetworkSAXHandler::context::context_type::interface_names_array:
                s << "interface names array";
                break;
            case NetworkSAXHandler::context::context_type::entry_array:
                s << "entry array";
                break;
            case NetworkSAXHandler::context::context_type::entry:
                s << "routing entry";
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
    constexpr NetworkSAXHandler::keys NetworkSAXHandler::context::get_key(NetworkSAXHandler::context::context_type context_type, NetworkSAXHandler::context::key_flag flag) {
        switch (context_type) {
            case NetworkSAXHandler::context::context_type::initial:
                if (flag == NetworkSAXHandler::context::key_flag::FLAG_1) {
                    return NetworkSAXHandler::keys::network;
                }
                break;
            case NetworkSAXHandler::context::context_type::network:
                switch (flag) {
                    case NetworkSAXHandler::context::key_flag::FLAG_1:
                        return NetworkSAXHandler::keys::network_name;
                    case NetworkSAXHandler::context::key_flag::FLAG_2:
                        return NetworkSAXHandler::keys::routers;
                    case NetworkSAXHandler::context::key_flag::FLAG_3:
                        return NetworkSAXHandler::keys::links;
                    default:
                        break;
                }
                break;
            case NetworkSAXHandler::context::context_type::link:
                switch (flag) {
                    case NetworkSAXHandler::context::key_flag::FLAG_1:
                        return NetworkSAXHandler::keys::from_interface;
                    case NetworkSAXHandler::context::key_flag::FLAG_2:
                        return NetworkSAXHandler::keys::from_router;
                    case NetworkSAXHandler::context::key_flag::FLAG_3:
                        return NetworkSAXHandler::keys::to_interface;
                    case NetworkSAXHandler::context::key_flag::FLAG_4:
                        return NetworkSAXHandler::keys::to_router;
                    default:
                        break;
                }
                break;
            case NetworkSAXHandler::context::context_type::router:
                switch (flag) {
                    case NetworkSAXHandler::context::key_flag::FLAG_1:
                        return NetworkSAXHandler::keys::router_name;
                    case NetworkSAXHandler::context::key_flag::FLAG_2:
                        return NetworkSAXHandler::keys::interfaces;
                    default:
                        break;
                }
                break;
            case NetworkSAXHandler::context::context_type::location:
                switch (flag) {
                    case NetworkSAXHandler::context::key_flag::FLAG_1:
                        return NetworkSAXHandler::keys::latitude;
                    case NetworkSAXHandler::context::key_flag::FLAG_2:
                        return NetworkSAXHandler::keys::longitude;
                    default:
                        break;
                }
                break;
            case NetworkSAXHandler::context::context_type::interface:
                switch (flag) {
                    case NetworkSAXHandler::context::key_flag::FLAG_1:
                        return NetworkSAXHandler::keys::interface_name; // NOTE: Also NetworkSAXHandler::keys::interface_names
                    case NetworkSAXHandler::context::key_flag::FLAG_2:
                        return NetworkSAXHandler::keys::routing_table;
                    default:
                        break;
                }
                break;
            case NetworkSAXHandler::context::context_type::entry:
                switch (flag) {
                    case NetworkSAXHandler::context::key_flag::FLAG_1:
                        return NetworkSAXHandler::keys::entry_out;
                    case NetworkSAXHandler::context::key_flag::FLAG_2:
                        return NetworkSAXHandler::keys::priority;
                    case NetworkSAXHandler::context::key_flag::FLAG_3:
                        return NetworkSAXHandler::keys::ops;
                    default:
                        break;
                }
                break;
            case NetworkSAXHandler::context::context_type::operation:
                if (flag == NetworkSAXHandler::context::key_flag::FLAG_1) {
                    return NetworkSAXHandler::keys::pop; // NOTE: Also NetworkSAXHandler::keys::swap and NetworkSAXHandler::keys::push
                }
                break;
            default:
                break;
        }
        assert(false);
        return NetworkSAXHandler::keys::unknown;
    }

    bool NetworkSAXHandler::pair_link(const std::string &from_router_name, const std::string &from_interface_name,
                                      const std::string &to_router_name, const std::string &to_interface_name,
                                      bool bidirectional, uint32_t link_weight) {
        auto [from_exists, from_id] = router_map.exists(from_router_name);
        if(!from_exists) {
            errors << "error: No router with name \"" << from_router_name << "\" was defined." << std::endl;
            return false;
        }
        auto from_router = router_map.get_data(from_id);

        auto [to_exists, to_id] = router_map.exists(to_router_name);
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
        from_interface->weight = link_weight;
        if (bidirectional) {
            to_interface->weight = link_weight;
        }
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
                current_link_bidirectional = value;
                break;
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
            case keys::swap:
                ops.emplace_back(RoutingTable::op_t::SWAP, value);
                break;
            case keys::push:
                ops.emplace_back(RoutingTable::op_t::PUSH, value);
                break;
            case keys::link_weight:
                current_link_weight = value;
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

    bool NetworkSAXHandler::add_router_name(const std::string& value) {
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

    bool NetworkSAXHandler::add_interface_name(const std::string& value) {
        auto [was_inserted, interface] = current_router->insert_interface(value, all_interfaces, false);
        if (!was_inserted) {
            // Was this added because of an out-interface in an already parsed routing-table?
            auto f = forward_constructed_interfaces.find(value);
            if (f != forward_constructed_interfaces.end()) {
                forward_constructed_interfaces.erase(f); // Then update set of pre constructed interfaces.
            } else { // Otherwise we have duplicate interface definition.
                if (current_router->names().empty()) {
                    errors << "error: Duplicate interface name \"" << value << "\" on router with index " << current_router->index() << "." << std::endl;
                } else {
                    errors << "error: Duplicate interface name \"" << value << "\" on router \"" << current_router->name() << "\"." << std::endl;
                }
                return false;
            }
        }
        current_interfaces.emplace_back(interface);
        return true;
    }

    bool NetworkSAXHandler::string(NetworkSAXHandler::string_t &value) {
        if (context_stack.empty()) {
            errors << "error: Unexpected string value: \"" << value << "\" outside of object." << std::endl;
            return false;
        }
        switch (context_stack.top().type) {
            case context::context_type::router_alias_array:
                return add_router_name(value);
            case context::context_type::interface_names_array:
                return add_interface_name(value);
            default:
                break;
        }
        switch (last_key){
            case keys::network_name:
                network_name = value;
                break;
            case keys::router_name:
                current_router_name = value;
                break;
            case keys::interface_name:
                return add_interface_name(value);
            case keys::entry_out: {
                auto [was_inserted, interface] = current_router->insert_interface(value, all_interfaces, false);
                if (was_inserted) {
                    forward_constructed_interfaces.insert(value);
                }
                via = interface;
                current_table_out_interfaces.emplace(via);
                break;
            }
            case keys::pop:
                assert(value.empty()); // TODO: Should this be error?
                ops.emplace_back(RoutingTable::op_t::POP);
                break;
            case keys::swap:
                ops.emplace_back(RoutingTable::op_t::SWAP, value);
                break;
            case keys::push:
                ops.emplace_back(RoutingTable::op_t::PUSH, value);
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

    bool NetworkSAXHandler::binary(binary_t&) {
        if (last_key == keys::unknown){
            return true;
        }
        errors << "error: Unexpected binary value found after key:" << last_key << std::endl;
        return false;
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
                current_table = current_router->emplace_table();
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

    template <NetworkSAXHandler::context::context_type type, NetworkSAXHandler::context::key_flag flag,
              NetworkSAXHandler::keys current_key, NetworkSAXHandler::keys... alternatives>
              // 'key' is the key to use. 'alternatives' are any other keys using the same flag in the same context (i.e. a one_of(key, alternatives...) requirement).
    bool NetworkSAXHandler::handle_key() {
        static_assert(flag == NetworkSAXHandler::context::FLAG_1 || flag == NetworkSAXHandler::context::FLAG_2
                   || flag == NetworkSAXHandler::context::FLAG_3 || flag == NetworkSAXHandler::context::FLAG_4,
                   "Template parameter flag must be a single key, not a union or empty.");
        static_assert(((NetworkSAXHandler::context::get_key(type, flag) == current_key) || ... || (NetworkSAXHandler::context::get_key(type, flag) == alternatives)),
                   "The result of get_key(type, flag) must match 'key' or one of the alternatives");
        if (!context_stack.top().needs_value(flag)) {
            errors << "Duplicate definition of key: \"" << current_key;
            ((errors << "\"/\"" << alternatives), ...);
            errors << "\" in " << type << " object. " << std::endl;
            return false;
        }
        context_stack.top().got_value(flag);
        last_key = current_key;
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
                    if (!handle_key<context::context_type::initial,context::FLAG_1,keys::network>()) return false;
                } else {
                    last_key = keys::unknown;
                }
                break;
            case context::context_type::network:
                if (key == "name") {
                    if (!handle_key<context::context_type::network,context::FLAG_1,keys::network_name>()) return false;
                } else if (key == "routers") {
                    if (!handle_key<context::context_type::network,context::FLAG_2,keys::routers>()) return false;
                } else if (key == "links") {
                    if (!handle_key<context::context_type::network,context::FLAG_3,keys::links>()) return false;
                } else { // "additionalProperties": true
                    last_key = keys::unknown;
                }
                break;
            case context::context_type::link:
                if (key == "from_interface") {
                    if (!handle_key<context::context_type::link,context::FLAG_1,keys::from_interface>()) return false;
                } else if (key == "from_router") {
                    if (!handle_key<context::context_type::link,context::FLAG_2,keys::from_router>()) return false;
                } else if (key == "to_interface") {
                    if (!handle_key<context::context_type::link,context::FLAG_3,keys::to_interface>()) return false;
                } else if (key == "to_router") {
                    if (!handle_key<context::context_type::link,context::FLAG_4,keys::to_router>()) return false;
                } else if (key == "bidirectional") {
                    last_key = keys::bidirectional;
                } else if (key == "weight") {
                    last_key = keys::link_weight;
                } else { // "additionalProperties": true
                    last_key = keys::unknown;
                }
                break;
            case context::context_type::router:
                if (key == "name") {
                    if (!handle_key<context::context_type::router,context::FLAG_1,keys::router_name>()) return false;
                } else if (key == "interfaces") {
                    if (!handle_key<context::context_type::router,context::FLAG_2,keys::interfaces>()) return false;
                } else if (key == "location") {
                    last_key = keys::location;
                } else if (key == "alias") {
                    last_key = keys::router_alias;
                } else {
                    errors << "Unexpected key in router object: " << key << std::endl;
                    return false;
                }
                break;
            case context::context_type::interface:
                if (key == "name") {
                    if (!handle_key<context::context_type::interface,context::FLAG_1,keys::interface_name, keys::interface_names>()) return false;
                } else if (key == "names") {
                    if (!handle_key<context::context_type::interface,context::FLAG_1,keys::interface_names, keys::interface_name>()) return false;
                } else if (key == "routing_table") {
                    if (!handle_key<context::context_type::interface,context::FLAG_2,keys::routing_table>()) return false;
                } else {
                    errors << "Unexpected key in interface object: " << key << std::endl;
                    return false;
                }
                break;
            case context::context_type::routing_table:
                last_key = keys::table_label;
                try {
                    current_table->emplace_entry(key);
                } catch (const std::invalid_argument& e) {
                    errors << "Invalid key in routing table object: " << key << std::endl;
                    return false;
                }
                break;
            case context::context_type::entry:
                if (key == "out") {
                    if (!handle_key<context::context_type::entry,context::FLAG_1,keys::entry_out>()) return false;
                } else if (key == "priority") {
                    if (!handle_key<context::context_type::entry,context::FLAG_2,keys::priority>()) return false;
                } else if (key == "ops") {
                    if (!handle_key<context::context_type::entry,context::FLAG_3,keys::ops>()) return false;
                } else if (key == "weight") {
                    last_key = keys::weight;
                } else {
                    errors << "Unexpected key in table entry object: " << key << std::endl;
                    return false;
                }
                break;
            case context::context_type::operation:
                if (key == "pop") {
                    if (!handle_key<context::context_type::operation,context::FLAG_1,keys::pop, keys::swap, keys::push>()) return false;
                } else if (key == "swap") {
                    if (!handle_key<context::context_type::operation,context::FLAG_1,keys::swap, keys::pop, keys::push>()) return false;
                } else if (key == "push") {
                    if (!handle_key<context::context_type::operation,context::FLAG_1,keys::push, keys::pop, keys::swap>()) return false;
                } else {
                    errors << "Unexpected key in operation object: " << key << std::endl;
                    return false;
                }
                break;
            case context::context_type::location:
                if (key == "latitude") {
                    if (!handle_key<context::context_type::location,context::FLAG_1,keys::latitude>()) return false;
                } else if (key == "longitude") {
                    if (!handle_key<context::context_type::location,context::FLAG_2,keys::longitude>()) return false;
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
            errors << "error: Missing key(s): ";
            bool first = true;
            for (const auto& flag : context::all_flags) {
                if (context_stack.top().needs_value(flag)) {
                    if (!first) errors << ", ";
                    first = false;
                    auto key = NetworkSAXHandler::context::get_key(context_stack.top().type, flag);
                    errors << key;
                    if (key == NetworkSAXHandler::keys::interface_name) {
                        errors << "/" << NetworkSAXHandler::keys::interface_names;
                    } else if (key == NetworkSAXHandler::keys::pop) {
                        errors << "/" << NetworkSAXHandler::keys::swap << "/" << NetworkSAXHandler::keys::push;
                    }
                }
            }
            errors << " in object: " << context_stack.top().type << std::endl;
            return false;
        }
        switch (context_stack.top().type) {
            case context::context_type::link:{
                if (routers_parsed) {
                    auto success = pair_link(current_from_router_name, current_from_interface_name, current_to_router_name, current_to_interface_name, current_link_bidirectional, current_link_weight);
                    if (!success) return false;
                } else {
                    links.emplace_back(current_from_router_name, current_from_interface_name, current_to_router_name, current_to_interface_name, current_link_bidirectional, current_link_weight);
                }
                // reset
                current_link_bidirectional = false;
                current_link_weight = std::numeric_limits<uint32_t>::max();
                break;
            }
            case context::context_type::router:
                if(!add_router_name(current_router_name)) {
                    return false;
                }
                if (!forward_constructed_interfaces.empty()) {
                    errors << "error: Interface " << *forward_constructed_interfaces.begin() << " used in a routing table is not defined on router " << current_router->name() << "." << std::endl ;
                    return false;
                }
                break;
            case context::context_type::location:
                current_router->set_coordinate(Coordinate(latitude, longitude));
                break;
            case context::context_type::interface: {
                current_table->set_out_interfaces(current_table_out_interfaces);
                for (Interface* current_interface : current_interfaces) {
                    current_interface->set_table(current_table);
                }
                current_table->sort(); current_table->sort_rules();
                current_interfaces.clear();
                current_table_out_interfaces.clear();
                break;
            }
            case context::context_type::entry:
                current_table->back()._rules.emplace_back(std::move(ops), via, priority, weight);
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
            case keys::router_alias:
                context_stack.push(router_alias_array);
                break;
            case keys::interface_names:
                context_stack.push(interface_names_array);
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
                for (const auto &[from_router, from_interface, to_router, to_interface, bidirectional, link_weight] : links) {
                    pair_link(from_router, from_interface, to_router, to_interface, bidirectional, link_weight);
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