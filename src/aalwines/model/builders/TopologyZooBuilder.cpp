//
// Created by dank on 4/6/20.
//

#include <fstream>
#include <sstream>
#include "TopologyZooBuilder.h"

namespace aalwines {

    Network aalwines::TopologyZooBuilder::parse(const std::string &gml) {
        std::ifstream file(gml);

        if(not file){
            std::cerr << "Error file not found." << std::endl;
        }

        size_t id;
        std::string longitude;
        std::string latitude;
        std::pair<double, double> coordinate;
        std::basic_string<char> router_name;

        size_t source;
        size_t target;
        std::string str;

        std::vector<std::pair<size_t, size_t >> _all_links;
        std::vector<std::pair<std::string, Coordinate>> _all_routers;
        std::map<const size_t, std::string> _router_map;

        while (std::getline(file, str, ' ')) {
            if(str =="node") {
                while (std::getline(file, str, ' ')){
                    if(str == "id"){
                        file >> id;
                    }
                    else if(str == "label"){
                        file >> router_name;
                        router_name.erase(router_name.begin() ,router_name.begin() + 1);
                        router_name.erase(router_name.end()-1, router_name.end());
                        //std::remove(router_name.begin(), router_name.end(), '"');
                    }
                    else if(str == "Longitude"){
                        std::getline(file, longitude);
                    }
                    else if(str == "Latitude"){
                        std::getline(file, latitude);
                    }
                    else if(str == "]\n"){
                        coordinate = std::make_pair( std::stod (longitude), std::stod (latitude));
                        _all_routers.emplace_back(std::make_pair(router_name, coordinate));
                        _router_map.insert({id, router_name});
                        break;
                    }
                }
            }
            else if(str == "edge"){
                while (std::getline(file, str, ' ')) {
                    if (str == "source") {
                        file >> source;
                    } else if (str == "target") {
                        file >> target;
                        _all_links.emplace_back(source, target);
                        break;
                    }
                }
            }
        }

        std::vector<std::vector<std::string>> _return_links;
        for(int i = 0; i < _all_routers.size(); i++){
            _return_links.emplace_back();
            for(auto& link : _all_links){
                if(link.first == i){
                    _return_links.back().emplace_back(_router_map[link.second]);
                }
            }
        }
        return Network::make_network(_all_routers, _return_links);
    }
}