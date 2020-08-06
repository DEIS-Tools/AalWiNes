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


#include <aalwines/model/experimental/Router.h>
#include <aalwines/model/experimental/Network.h>

using json = nlohmann::json;
namespace nlohmann {
    template <>
    struct adl_serializer<aalwines::Coordinate> {
        static aalwines::Coordinate from_json(const json& j) {
            auto latitude = j.at("latitude").get<double>();
            auto longitude = j.at("longitude").get<double>();
            return aalwines::Coordinate(latitude, longitude);
        }
        static void to_json(json& j, aalwines::Coordinate t) {
            j = json::object();
            j["latitude"] = t.latitude();
            j["longitude"] = t.longitude();
        }
    };
}

namespace aalwines {
    using Network = aalwines::experimental::Network;
    using Router = aalwines::experimental::Router;
    using Interface = aalwines::experimental::Interface;
    using RoutingTable = aalwines::experimental::RoutingTable;


    void from_json(const json & j, RoutingTable::action_t& action);
    void to_json(json & j, const RoutingTable::action_t& action);


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



#endif //AALWINES_AALWINESBUILDER_H
