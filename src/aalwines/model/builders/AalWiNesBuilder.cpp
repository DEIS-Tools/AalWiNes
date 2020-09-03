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
        std::vector<std::unique_ptr<Router>> routers;
        std::vector<const Interface*> all_interfaces;
        Network::routermap_t mapping;

        std::stringstream es;

        auto network_name = json_network.at("name").get<std::string>();

        auto json_routers = json_network.at("routers");
        if (!json_routers.is_array()) {
            throw base_error("error: routers field is not an array.");
        }
        for (const auto& json_router : json_routers) {
            auto names = json_router.at("names").get<std::vector<std::string>>();
            if (names.empty()) {
                throw base_error("error: Router must have at least one name.");
            }
            size_t id = routers.size();

            auto loc_it = json_router.find("location");
            auto coordinate = loc_it != json_router.end() ? std::optional<Coordinate>(loc_it->get<Coordinate>()) : std::nullopt;
            routers.emplace_back(std::make_unique<Router>(id, names, coordinate));
            auto router = routers.back().get();
            for (const auto& name : names) {
                auto res = mapping.insert(name.c_str(), name.length());
                if(!res.first)
                {
                    es << "error: Duplicate definition of \"" << name << "\", previously found in entry " << mapping.get_data(res.second)->index() << std::endl;
                    throw base_error(es.str());
                }
                mapping.get_data(res.second) = router;
            }

            auto json_interfaces = json_router.at("interfaces");
            if (!json_interfaces.is_array()) {
                es << "error: interfaces field of router \"" << names[0] << "\" is not an array.";
                throw base_error(es.str());
            }
            // First create all interfaces.
            for (const auto& json_interface : json_interfaces) {
                auto interface_name = json_interface.at("name").get<std::string>();
                router->get_interface(all_interfaces, interface_name);
            }
            // Then create routing tables for the interfaces.
            for (const auto& json_interface : json_interfaces) {
                auto interface_name = json_interface.at("name").get<std::string>();;

                auto json_routing_table = json_interface.at("routing_table");
                if (!json_routing_table.is_object()) {
                    es << "error: routing table field for interface \"" << interface_name << "\" of router \"" << names[0] << "\" is not an object.";
                    throw base_error(es.str());
                }

                auto interface = router->find_interface(interface_name);
                for (const auto& [label_string, json_routing_entries] : json_routing_table.items()) {
                    auto& entry = interface->table().emplace_entry(RoutingTable::label_t(label_string));

                    if (!json_routing_entries.is_array()) {
                        es << "error: Value of routing table entry \"" << label_string << "\" is not an array. In interface \"" << interface_name << "\" of router \"" << names[0] << "\"." << std::endl;
                        throw base_error(es.str());
                    }
                    for (const auto& json_routing_entry : json_routing_entries) {
                        auto via = router->find_interface(json_routing_entry.at("out").get<std::string>());
                        auto ops = json_routing_entry.at("ops").get<std::vector<RoutingTable::action_t>>();
                        auto priority = json_routing_entry.at("priority").get<size_t>();
                        auto weight = json_routing_entry.contains("weight") ? json_routing_entry.at("weight").get<uint32_t>() : 0;
                        entry._rules.emplace_back(RoutingTable::type_t::MPLS, std::move(ops), via, priority, weight);
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

            auto from_res = mapping.exists(from_router_name.c_str(), from_router_name.length());
            auto to_res = mapping.exists(to_router_name.c_str(), to_router_name.length());
            if(!from_res.first) {
                es << "error: No router with name \"" << from_router_name << "\" was defined." << std::endl;
                throw base_error(es.str());
            }
            if(!to_res.first) {
                es << "error: No router with name \"" << to_router_name << "\" was defined." << std::endl;
                throw base_error(es.str());
            }
            auto from_interface = mapping.get_data(from_res.second)->find_interface(from_interface_name);
            auto to_interface = mapping.get_data(to_res.second)->find_interface(to_interface_name);
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

        Router::add_null_router(routers, all_interfaces, mapping);
        Network network(std::move(mapping), std::move(routers), std::move(all_interfaces));
        network.name = network_name;
        return network;
    }

}