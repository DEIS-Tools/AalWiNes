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
 * File:   AalWiNesBuilder.cpp
 * Author: Morten K. Schou <morten@h-schou.dk>
 *
 * Created on August 5, 2020
 */

#include "AalWiNesBuilder.h"


namespace aalwines {


    Network AalWiNesBuilder::parse(const json& json_network, std::ostream& warnings) {
        auto network_name = json_network.at("name").get<std::string>();
        Network network(network_name);

        std::stringstream es;
        auto json_routers = json_network.at("routers");
        if (!json_routers.is_array()) {
            throw base_error("error: routers field is not an array.");
        }
        for (const auto& json_router : json_routers) {
            auto names = json_router.contains("alias") ? json_router.at("alias").get<std::vector<std::string>>() : std::vector<std::string>();
            names.emplace_back(json_router.at("name").get<std::string>());
            auto loc_it = json_router.find("location");
            auto coordinate = loc_it != json_router.end() ? std::optional<Coordinate>(loc_it->get<Coordinate>()) : std::nullopt;

            auto router = network.add_router(names, coordinate);

            auto json_interfaces = json_router.at("interfaces");
            if (!json_interfaces.is_array()) {
                es << "error: interfaces field of router \"" << names.back() << "\" is not an array." << std::endl;
                throw base_error(es.str());
            }
            // First create all interfaces.
            for (const auto& json_interface : json_interfaces) {
                std::vector<std::string> interface_names;
                if (json_interface.contains("name")) {
                    interface_names.emplace_back(json_interface.at("name").get<std::string>());
                }
                if (json_interface.contains("names")) {
                    if (!interface_names.empty()) {
                        es << "error: you cannot specify both 'name' and 'names' for interface \"" << interface_names.back() << "\" of router \"" << names.back() << "\"." << std::endl;
                        throw base_error(es.str());
                    }
                    interface_names = json_interface.at("names").get<std::vector<std::string>>();
                }
                if (interface_names.empty()) {
                    es << "error: missing interface name(s) on router \"" << names.back() << "\"." << std::endl;
                    throw base_error(es.str());
                }
                for (const auto& interface_name : interface_names) {
                    auto [was_inserted, _] = network.insert_interface_to(interface_name, router);
                    if (!was_inserted) {
                        es << "error: Duplicate interface name \"" << interface_name << "\" on router \"" << names.back() << "\"." << std::endl;
                        throw base_error(es.str());
                    }
                }
            }
            // Then create routing tables for the interfaces.
            for (const auto& json_interface : json_interfaces) {
                std::vector<std::string> interface_names = json_interface.contains("name")
                        ? std::vector<std::string>{json_interface.at("name").get<std::string>()}
                        : json_interface.at("names").get<std::vector<std::string>>();
                assert(!interface_names.empty());
                auto json_routing_table = json_interface.at("routing_table");
                if (!json_routing_table.is_object()) {
                    es << "error: routing table field for interface \"" << interface_names[0] << "\" of router \"" << names.back() << "\" is not an object." << std::endl;
                    throw base_error(es.str());
                }

                for (const auto& interface_name : interface_names) {
                    auto interface = router->find_interface(interface_name);
                    // TODO: This construction can be optimized. Currently we copy shared routing tables onto each interface.
                    for (const auto& [label_string, json_routing_entries] : json_routing_table.items()) {
                        auto& entry = interface->table().emplace_entry(RoutingTable::label_t(label_string));

                        if (!json_routing_entries.is_array()) {
                            es << "error: Value of routing table entry \"" << label_string << "\" is not an array. In interface \"" << interface_name << "\" of router \"" << names.back() << "\"." << std::endl;
                            throw base_error(es.str());
                        }
                        for (const auto& json_routing_entry : json_routing_entries) {
                            auto via = router->find_interface(json_routing_entry.at("out").get<std::string>());
                            auto ops = json_routing_entry.at("ops").get<std::vector<RoutingTable::action_t>>();
                            auto priority = json_routing_entry.at("priority").get<size_t>();
                            auto weight = json_routing_entry.contains("weight") ? json_routing_entry.at("weight").get<uint32_t>() : 0;
                            entry._rules.emplace_back(std::move(ops), via, priority, weight);
                        }
                    }
                }
            }
        }

        auto json_links = json_network.at("links");
        if (!json_links.is_array()) {
            throw base_error("error: links field is not an array.");
        }
        for (const auto& json_link : json_links) {
            // auto temp_it = json_link.find("bidirectional");
            // bool bidirectional = temp_it != json_link.end() && temp_it->get<bool>();

            auto from_interface_name = json_link.at("from_interface").get<std::string>();
            auto from_router_name = json_link.at("from_router").get<std::string>();
            auto to_interface_name = json_link.at("to_interface").get<std::string>();
            auto to_router_name = json_link.at("to_router").get<std::string>();

            auto from_router = network.find_router(from_router_name);
            auto to_router = network.find_router(to_router_name);
            if(from_router == nullptr) {
                es << "error: No router with name \"" << from_router_name << "\" was defined." << std::endl;
                throw base_error(es.str());
            }
            if(to_router == nullptr) {
                es << "error: No router with name \"" << to_router_name << "\" was defined." << std::endl;
                throw base_error(es.str());
            }
            auto from_interface = from_router->find_interface(from_interface_name);
            auto to_interface = to_router->find_interface(to_interface_name);
            if (from_interface == nullptr) {
                es << "error: No interface with name \"" << from_interface_name << "\" was defined for router \"" << from_router_name << "\"." << std::endl;
                throw base_error(es.str());
            }
            if (to_interface == nullptr) {
                es << "error: No interface with name \"" << to_interface_name << "\" was defined for router \"" << to_router_name << "\"." << std::endl;
                throw base_error(es.str());
            }
            if ((from_interface->match() != nullptr && from_interface->match() != to_interface) || (to_interface->match() != nullptr && to_interface->match() != from_interface)) {
                es << "error: Conflicting link: " << json_link << std::endl;
                throw base_error(es.str());
            }
            from_interface->make_pairing(to_interface);
        }

        network.add_null_router();

        return network;
    }

}