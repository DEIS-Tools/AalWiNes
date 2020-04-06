//
// Created by dank on 4/6/20.
//

#include <fstream>
#include <sstream>
#include "TopologyZooBuilder.h"

namespace aalwines {

    Network aalwines::TopologyZooBuilder::parse(const std::string &gml, std::ostream &warnings) {
        std::ifstream file(gml);

        if(not file){
            std::cerr << "Error file not found." << std::endl;
        }

        std::string id;
        std::string longitude;
        std::string latitude;
        std::pair<double, double> coordinates;
        std::string router_name;
        std::vector<parse_router> routers;
        std::vector<parse_link> links;

        std::string source;
        std::string target;
        std::string str;

        while (std::getline(file, str, ' ')) {
            if(str =="node") {
                while (std::getline(file, str, ' ')){
                    if(str == "id"){
                        std::getline(file, id);
                    }
                    else if(str == "label"){
                        std::getline(file, router_name);
                        std::remove(router_name.begin(), router_name.end(), '"');
                    }
                    else if(str == "Longitude"){
                        std::getline(file, longitude);
                    }
                    else if(str == "Latitude"){
                        std::getline(file, latitude);
                    }
                    else if(str == "]\n"){
                        coordinates = std::make_pair( std::stod (longitude), std::stod (latitude));
                        routers.emplace_back(parse_router(std::stod (id), router_name, coordinates));
                        break;
                    }
                }
            }
            else if(str == "edge"){
                while (std::getline(file, str, ' ')) {
                    if (str == "source") {
                        std::getline(file, source);
                    } else if (str == "target") {
                        std::getline(file, target);
                        links.emplace_back(parse_link(std::stod(source), std::stod(target)));
                        break;
                    }
                }
            }
        }
        //return Network::make_network();
    }
}