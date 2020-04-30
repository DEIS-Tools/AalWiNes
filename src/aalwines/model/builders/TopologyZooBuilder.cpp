//
// Created by Dan Kristiansen on 4/6/20.
//

#include <fstream>
#include <sstream>
#include "TopologyZooBuilder.h"

namespace aalwines {

    // Stolen from: https://stackoverflow.com/questions/216823/whats-the-best-way-to-trim-stdstring
    // trim from start (in place)
    static inline void ltrim(std::string &s) {
        s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](int ch) {
            return !std::isspace(ch);
        }));
    }
    // trim from end (in place)
    static inline void rtrim(std::string &s) {
        s.erase(std::find_if(s.rbegin(), s.rend(), [](int ch) {
            return !std::isspace(ch);
        }).base(), s.end());
    }
    // trim from both ends (in place)
    static inline void trim(std::string &s) {
        ltrim(s);
        rtrim(s);
    }

    Network aalwines::TopologyZooBuilder::parse(const std::string &gml) {
        std::ifstream file(gml);

        if(not file){
            std::cerr << "Error file not found." << std::endl;
        }

        std::vector<std::pair<size_t, size_t >> _all_links;
        std::vector<std::pair<std::string, Coordinate>> _all_routers;
        std::map<const size_t, std::string> _router_map;
        std::vector<std::vector<std::string>> _return_links;

        std::string str;
        while (std::getline(file, str, ' ')) {
            if(str =="node") {
                size_t id;
                std::string router_name;
                double latitude = 0;
                double longitude = 0;
                while (std::getline(file, str)){
                    trim(str);
                    auto pos = str.find(' ');
                    if (pos == str.size()) continue;
                    std::string key = str.substr(0, pos);
                    if(key == "id"){
                        id = std::stoul(str.substr(pos+1));
                    }
                    else if(key == "label"){
                        router_name = str.substr(pos+2, str.size() - pos - 3);
                        trim(router_name);
                        //Get_interface dont handle ' ' very well
                        std::replace(router_name.begin(), router_name.end(), ' ', '_');
                        router_name.erase(std::remove_if(router_name.begin(), router_name.end(),
                                [](auto const& c) -> bool { return !std::isalnum(c) && c != '_' && c != '-'; }), router_name.end());
                    }
                    else if(key == "Latitude"){
                        latitude = std::stod(str.substr(pos+1));
                    }
                    else if(key == "Longitude"){
                        longitude = std::stod(str.substr(pos+1));
                    }
                    else if(key == "]"){
                        bool indexer_active = false;
                        while (std::find_if( _all_routers.begin(), _all_routers.end(),
                                [&]( std::pair<std::string,Coordinate> &item ) { return item.first == router_name; } )
                                != _all_routers.end()) {
                            if (indexer_active){
                                if ((int) router_name[router_name.size()-1]++ - 48 > 5) assert(false);
                            } else {
                                router_name += "2";
                                indexer_active = true;
                            }
                        }
                        assert(latitude != 0 or longitude != 0);
                        _all_routers.emplace_back(router_name, Coordinate{latitude, longitude});
                        _router_map.insert({id, router_name});
                        _return_links.emplace_back();
                        break;
                    }
                }
            }
            else if(str == "edge"){
                size_t source;
                size_t target;
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

        for(size_t i = 0; i < _all_routers.size(); i++){
            for(auto& link : _all_links){
                if(link.first == i){
                    _return_links[i].emplace_back(_router_map[link.second]);
                    _return_links[link.second].emplace_back(_router_map[link.first]);
                }
            }
        }
        return Network::make_network(_all_routers, _return_links);
    }
}