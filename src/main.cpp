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

#include "model/Router.h"

#include <ptrie/ptrie_map.h>

#include <boost/program_options.hpp>

#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#include <memory.h>

namespace po = boost::program_options;

int main(int argc, const char** argv)
{
    po::options_description opts;
    std::string network;
    bool print_dot = false;
    opts.add_options()
            ("help,h", "produce help message")
            ("network,n", po::value<std::string>(&network),
                "A file containing a network-description; each line is a router in the format \"name,alias1,alias2:adjacency.xml,mpls.xml\". ")
            ("dot", po::bool_switch(&print_dot), "A dot output will be printed to cout when set.")
    ;

    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, opts), vm);
    po::notify(vm);
    
    if (vm.count("help")) {
        std::cout << opts << "\n";
        return 1;
    }
    std::ifstream data(network);
    if(!data.is_open())
    {
        std::cerr << "Could not open " << network << std::endl;
        exit(-1);
    }
    
    // lets start by creating empty router-objects for all the alias' we have
    ptrie::map<Router*> mapping;
    std::vector<std::unique_ptr<Router>> routers;
    std::vector<std::pair<std::string,std::string>> configs;
    std::string line;
    while(getline(data, line))
    {
        if(line.size() == 0)
            continue;
        size_t en = 0;
        for(; en < line.length(); ++en) if(line[en] == ':') break;
        std::string alias = line.substr(0, en);
        {
            auto id = routers.size();
            routers.emplace_back(std::make_unique<Router>(id));
            std::string tmp;
            bool some = false;
            std::istringstream ss(alias);
            while(std::getline(ss, tmp, ','))
            {
                if(tmp.size() == 0) continue;
                some = true;
                auto res = mapping.insert((unsigned char*)tmp.c_str(), tmp.size());
                if(!res.first)
                {
                    auto oid = mapping.get_data(res.second)->index();
                    if(oid != id)
                    {
                        std::cerr << "error: Duplicate definition of \"" << tmp << "\", previously found in entry " << oid << std::endl;
                        exit(-1);
                    }
                    else
                    {
                        std::cerr << "Warning: entry " << id << " contains the duplicate alias \"" << tmp << "\"" << std::endl; 
                        continue;
                    }
                }
                mapping.get_data(res.second) = routers.back().get();
                routers.back()->add_name(tmp);
            }
            if(!some)
            {
                std::cerr << "error: Empty name-string declared for entry " << id << std::endl;
                exit(-1);
            }
            configs.emplace_back();
            if(en + 1 < line.size())
            {
                ++en;
                size_t config = en;
                for(;config < line.size(); ++config) if(line[config] == ':') break;
                configs.back().first = line.substr(en, (config - en));
                ++config;
                en = line.size();
                if(line.back() == ':') --en;
                if(en > config)
                    configs.back().second = line.substr(config, (en-config));
                if(configs.back().first.empty() || configs.back().second.empty())
                {
                    std::cerr << "error: Either no configuration files are specified, or both adjacency and mpls is specified." << std::endl;
                    return false;
                }
            }
        }
    }
    
    for(size_t i = 0; i < configs.size(); ++i)
    {
        if(configs[i].first.empty()) continue;
        std::ifstream stream(configs[i].first);
        if(!stream.is_open())
        {
            std::cerr << "error: Could not open adjacency-description for index " << i << " (i.e. router " << routers[i]->name() << ")" << std::endl;
            exit(-1);
        }
        if(!routers[i]->parse_adjacency(stream, routers, mapping))
        {
            exit(-1);
        }
    }
    
    if(print_dot)
    {
        std::cout << "digraph network {\n";
        for(auto& r : routers)
        {
            r->print_dot(std::cout);
        }
        std::cout << "}" << std::endl;
    }
    
    return 0;
}
