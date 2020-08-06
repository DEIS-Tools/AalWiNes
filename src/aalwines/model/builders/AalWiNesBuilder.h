//
// Created by Morten on 05-08-2020.
//

#ifndef AALWINES_AALWINESBUILDER_H
#define AALWINES_AALWINESBUILDER_H



//  Then include this file, and then do
//
//     Network data = nlohmann::json::parse(jsonString);
#include <json.hpp>
#include <fstream>
#include <sstream>


#include <aalwines/model/Router.h>
#include <aalwines/model/Network.h>

using json = nlohmann::json;
namespace nlohmann {
    template <typename T>
    struct adl_serializer<std::unique_ptr<T>> {
        static std::unique_ptr<T> from_json(const json& j) {
            return std::make_unique<T>(j.get<T>());
        }
        static void to_json(json& j, const std::unique_ptr<T>& p) {
            j = *p.get();
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
    void from_json(const json & j, RoutingTable::action_t& action);
    void to_json(json & j, const RoutingTable::action_t& action);
    void to_json(json & j, const RoutingTable::forward_t& rule);
    void to_json(json & j, const RoutingTable& table);
    void to_json(json & j, const Interface& interface);
    void to_json(json & j, const Router& router);

    class AalWiNesBuilder {
    public:
        static Network parse(const json &j, std::ostream &warnings);

        static Network parse(std::istream &stream, std::ostream &warnings) {
            json j;
            stream >> j;
            return parse(j, warnings);
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
            j["routers"] = network.routers();

            auto links_j = json::array();
            for (const auto& interface : network.all_interfaces()){
                if (interface->match() == nullptr) continue; // Not connected
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
