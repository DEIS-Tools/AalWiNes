//
// Created by Morten on 05-08-2020.
//

#include "AalWiNesBuilder.h"


namespace aalwines {

    inline void from_json(const json & j, RoutingTable::action_t& action) {
        if (!j.is_object()){
            throw base_error("error: Operation is not an object");
        }
        int i = 0;
        for (const auto& [op_string, label_string] : j.items()) {
            if (i > 0){
                throw base_error("error: Operation object must contain exactly one property");
            }
            i++;
            if (op_string == "pop") {
                action._op = RoutingTable::op_t::POP;
            } else if (op_string == "swap") {
                action._op = RoutingTable::op_t::SWAP;
                action._op_label.set_value(label_string);
            } else if (op_string == "push") {
                action._op = RoutingTable::op_t::PUSH;
                action._op_label.set_value(label_string);
            } else {
                std::stringstream es;
                es << "error: Unknown operation: \"" << op_string << "\": \"" << label_string << "\"" << std::endl;
                throw base_error(es.str());
            }
        }
        if (i == 0) {
            throw base_error("error: Operation object was empty");
        }
    }

    inline void to_json(json & j, const RoutingTable::action_t& action) {
        j = json::object();
        switch (action._op) {
            default: // TODO: Make RoutingTable::op_t enum class and remove this default:
            case RoutingTable::op_t::POP:
                j["pop"] = "";
                break;
            case RoutingTable::op_t::SWAP: {
                std::stringstream label;
                label << action._op_label; // TODO: Is this format correct?
                j["swap"] = label.str();
                break;
            }
            case RoutingTable::op_t::PUSH: {
                std::stringstream label;
                label << action._op_label; // TODO: Is this format correct?
                j["push"] = label.str();
                break;
            }
        }
    }

    inline void to_json(json & j, const RoutingTable::forward_t& rule) {
        j = json::object();
        j["out"] = rule._via->get_name();
        j["priority"] = rule._weight;
        j["ops"] = rule._ops;
        if (rule._custom_weight != 0) {
            j["weight"] = rule._custom_weight;
        }
    }

    inline void to_json(json & j, const RoutingTable& table) {
        j = json::object();
        for (const auto& entry : table.entries()) {
            std::stringstream label;
            label << entry._top_label; // TODO: Is this correct format??
            j[label.str()] = entry._rules;
        }
    }
    inline void to_json(json & j, const Interface& interface) {
        j = json::object();
        j["name"] = interface.get_name();
        j["routing_table"] = interface.table();
    }

    inline void to_json(json & j, const Router& router) {
        j = json::object();
        j["names"] = router.names();
        j["interfaces"] = router.interfaces();
        if (router.coordinate()) {
            j["location"] = router.coordinate().value();
        }
    }


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
                    auto entry = interface->table().emplace_entry(RoutingTable::label_t(label_string));

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
                es << "error: "; // error TODO
                throw base_error(es.str());
            }
            if(!to_res.first) {
                es << "error: "; // error TODO
                throw base_error(es.str());
            }
            auto from_interface = mapping.get_data(from_res.second)->find_interface(from_interface_name);
            auto to_interface = mapping.get_data(to_res.second)->find_interface(to_interface_name);
            if (from_interface == nullptr) {
                es << "error: "; // error TODO
                throw base_error(es.str());
            }
            if (to_interface == nullptr) {
                es << "error: "; // error TODO
                throw base_error(es.str());
            }
            if ((from_interface->match() != nullptr && from_interface->match() != to_interface) || (to_interface->match() != nullptr && to_interface->match() != from_interface)) {
                es << "error: "; // error TODO
                throw base_error(es.str());
            }
            from_interface->make_pairing(to_interface);
        }

        Network network(std::move(mapping), std::move(routers), std::move(all_interfaces));
        network.name = network_name;
        return network;
    }

}