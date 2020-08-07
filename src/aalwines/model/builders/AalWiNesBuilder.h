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
 * File:   AalWiNesBuilder.h
 * Author: Morten K. Schou <morten@h-schou.dk>
 *
 * Created on August 5, 2020
 */
#ifndef AALWINES_AALWINESBUILDER_H
#define AALWINES_AALWINESBUILDER_H


#include <json.hpp>
#include <fstream>
#include <sstream>
#include <aalwines/model/Router.h>
#include <aalwines/model/Network.h>

using json = nlohmann::json;

namespace aalwines {
    void from_json(const json& j, RoutingTable::action_t& action);
    void to_json(json& j, const RoutingTable::action_t& action);
    void to_json(json& j, const RoutingTable::forward_t& rule);
    void to_json(json& j, const RoutingTable& table);
    void to_json(json& j, const Interface& interface);
    void to_json(json& j, const Router& router);
}

namespace nlohmann {

    template <typename T>
    struct adl_serializer<std::unique_ptr<T>> {
        static std::unique_ptr<T> from_json(const json& j) {
            return std::make_unique<T>(j.get<T>());
        }
        static void to_json(json& j, const std::unique_ptr<T>& p) {
            nlohmann::to_json(j, *p);
        }
    };

    template <>
    struct adl_serializer<aalwines::Coordinate> {
        static aalwines::Coordinate from_json(const json& j) {
            auto latitude = j.at("latitude").get<double>();
            auto longitude = j.at("longitude").get<double>();
            return aalwines::Coordinate(latitude, longitude);
        }
        static void to_json(json& j, const aalwines::Coordinate& c) {
            j = json::object();
            j["latitude"] = c.latitude();
            j["longitude"] = c.longitude();
        }
    };
}

namespace aalwines {
    class AalWiNesBuilder {
    public:
        static Network parse(const json &j, std::ostream &warnings);

        static Network parse(std::istream &stream, std::ostream &warnings) {
            json j;
            stream >> j;
            return parse(j.at("network"), warnings);
        }

        static Network parse(const std::string &network_file, std::ostream &warnings) {
            std::ifstream stream(network_file);
            if (!stream.is_open()) {
                std::stringstream es;
                es << "error: Could not open file : " << network_file << std::endl;
                throw base_error(es.str());
            }
            return parse(stream, warnings);
        }
    };

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
                label << action._op_label;
                j["swap"] = label.str();
                break;
            }
            case RoutingTable::op_t::PUSH: {
                std::stringstream label;
                label << action._op_label;
                j["push"] = label.str();
                break;
            }
        }
    }

    inline void to_json(json & j, const RoutingTable::forward_t& rule) {
        j = json::object();
        j["out"] = rule._via->get_name();
        j["priority"] = rule._priority;
        j["ops"] = rule._ops;
        if (rule._custom_weight != 0) {
            j["weight"] = rule._custom_weight;
        }
    }

    inline void to_json(json & j, const RoutingTable& table) {
        j = json::object();
        for (const auto& entry : table.entries()) {
            std::stringstream label;
            label << entry._top_label;
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
}

namespace nlohmann {
    template <>
    struct adl_serializer<aalwines::Network> {
        static aalwines::Network from_json(const json& j) {
            return aalwines::AalWiNesBuilder::parse(j, std::cerr);
        }
        static void to_json(json& j, const aalwines::Network& network) {
            j = json::object();
            j["name"] = network.name;
            j["routers"] = json::array();
            for (const auto& router : network.routers()) {
                if (router->is_null()) continue;
                j["routers"].push_back(router);
            }

            auto links_j = json::array();
            for (const auto& interface : network.all_interfaces()){
                if (interface->match() == nullptr) continue; // Not connected
                if (interface->source()->is_null() || interface->target()->is_null()) continue; // Skip the NULL router
                if (interface->match()->table().empty()) continue; // Not this direction
                assert(interface->match()->match() == interface);
                bool bidirectional = !interface->table().empty();
                if (interface->global_id() > interface->match()->global_id() && bidirectional) continue; // Already covered by bidirectional link the other way.

                auto link_j = json::object();
                link_j["from_interface"] = interface->get_name();
                link_j["from_router"] = interface->source()->name();
                link_j["to_interface"] = interface->match()->get_name();
                link_j["to_router"] = interface->target()->name();
                if (bidirectional) {
                    link_j["bidirectional"] = true;
                }
                links_j.push_back(link_j);
            }
            j["links"] = links_j;
        }
    };
}


#endif //AALWINES_AALWINESBUILDER_H
