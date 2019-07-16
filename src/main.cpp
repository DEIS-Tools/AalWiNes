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
#include "utils/errors.h"
#include "parser/QueryBuilder.h"

#include <ptrie_map.h>

#include <boost/program_options.hpp>

#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#include <memory.h>

namespace po = boost::program_options;

void parse_junos(std::vector<std::unique_ptr<Router>>&routers,
                 ptrie::map<Router*>& mapping,
                 const std::string& network,
                 std::ostream& warnings)
{
    // lets start by creating empty router-objects for all the alias' we have
    using tp = std::tuple<std::string, std::string, std::string>;
    std::vector<tp> configs;
    std::string line;
    std::ifstream data(network);
    if (!data.is_open()) {
        std::cerr << "Could not open " << network << std::endl;
        exit(-1);
    }

    while (getline(data, line)) {
        if (line.size() == 0)
            continue;
        size_t en = 0;
        for (; en < line.length(); ++en) if (line[en] == ':') break;
        std::string alias = line.substr(0, en);
        {
            auto id = routers.size();
            routers.emplace_back(std::make_unique<Router>(id));
            std::string tmp;
            bool some = false;
            std::istringstream ss(alias);
            while (std::getline(ss, tmp, ',')) {
                if (tmp.size() == 0) continue;
                some = true;
                auto res = mapping.insert((unsigned char*) tmp.c_str(), tmp.size());
                if (!res.first) {
                    auto oid = mapping.get_data(res.second)->index();
                    if (oid != id) {
                        std::cerr << "error: Duplicate definition of \"" << tmp << "\", previously found in entry " << oid << std::endl;
                        exit(-1);
                    }
                    else {
                        warnings << "Warning: entry " << id << " contains the duplicate alias \"" << tmp << "\"" << std::endl;
                        continue;
                    }
                }
                mapping.get_data(res.second) = routers.back().get();
                routers.back()->add_name(tmp);
            }
            if (!some) {
                std::cerr << "error: Empty name-string declared for entry " << id << std::endl;
                exit(-1);
            }
            configs.emplace_back();
            if (en + 1 < line.size()) {
                // TODO: cleanup this pasta.
                ++en;
                size_t config = en;
                for (; config < line.size(); ++config) if (line[config] == ':') break;
                if (en < config)
                    std::get<0>(configs.back()) = line.substr(en, (config - en));
                ++config;
                en = config;
                for (; config < line.size(); ++config) if (line[config] == ':') break;
                if (en < config)
                    std::get<1>(configs.back()) = line.substr(en, (config - en));
                ++config;
                en = config;
                for (; config < line.size(); ++config) if (line[config] == ':') break;
                if (en < config)
                    std::get<2>(configs.back()) = line.substr(en, (config - en));

                if (std::get<0>(configs.back()).empty() || std::get<1>(configs.back()).empty()) {
                    std::cerr << "error: Either no configuration files are specified, or both adjacency and mpls is specified." << std::endl;
                    std::cerr << line << std::endl;
                    exit(-1);
                }
                if (!std::get<2>(configs.back()).empty() && (std::get<1>(configs.back()).empty() || std::get<0>(configs.back()).empty())) {
                    std::cerr << "error: next-hop-table requires definition of other configuration-files." << std::endl;
                    std::cerr << line << std::endl;
                    exit(-1);
                }
            }
        }
    }
    for (size_t i = 0; i < configs.size(); ++i) {
        if (std::get<0>(configs[i]).empty()) {
            warnings << "warning: No adjacency info for index " << i << " (i.e. router " << routers[i]->name() << ")" << std::endl;
        }
        else {
            std::ifstream stream(std::get<0>(configs[i]));
            if (!stream.is_open()) {
                std::cerr << "error: Could not open adjacency-description for index " << i << " (i.e. router " << routers[i]->name() << ")" << std::endl;
                exit(-1);
            }
            try {
                routers[i]->parse_adjacency(stream, routers, mapping, warnings);
            }
            catch (base_error& ex) {
                std::cerr << ex.what() << "\n";
                std::cerr << "while parsing : " << std::get<0>(configs[i]) << std::endl;
                stream.close();
                exit(-1);
            }
            stream.close();
        }
        if (std::get<1>(configs[i]).empty()) {
            warnings << "warning: No routingtables for index " << i << " (i.e. router " << routers[i]->name() << ")" << std::endl;
        }
        else {
            std::ifstream stream(std::get<1>(configs[i]));
            if (!stream.is_open()) {
                std::cerr << "error: Could not open routing-description for index " << i << " (i.e. router " << routers[i]->name() << ")" << std::endl;
                exit(-1);
            }
            std::ifstream id;
            if (!std::get<2>(configs[i]).empty()) {
                id.open(std::get<2>(configs[i]));
            }
            try {
                routers[i]->parse_routing(stream, id, warnings);
            }
            catch (base_error& ex) {
                std::cerr << ex.what() << "\n";
                std::cerr << "while parsing : " << std::get<0>(configs[i]) << std::endl;
                stream.close();
                exit(-1);
            }
            stream.close();
        }
    }
}

/*
 TODO:
 * Sep. networkmodel from parsing
 * Move parsing of junos into sep. structure
 * fix error-handling
 * improve on throws/exception-types
 * fix slots
 */
int main(int argc, const char** argv)
{
    po::options_description opts;
    opts.add_options()
            ("help,h", "produce help message");
    
    po::options_description output("Output Options");
    po::options_description input("Input Options");
    po::options_description verification("Verification Options");    
    
    bool print_dot = false;
    bool dump_json = false;
    bool no_parser_warnings = false;
    output.add_options()
            ("dot", po::bool_switch(&print_dot), "A dot output will be printed to cout when set.")
            ("json", po::bool_switch(&dump_json), "A json output will be printed to cout when set.")
            ("disable-parser-warnings,W", po::bool_switch(&no_parser_warnings), "Disable warnings from parser.")
            ;

    std::string junos_config;
    input.add_options()
            ("juniper,j", po::value<std::string>(&junos_config),
            "A file containing a network-description; each line is a router in the format \"name,alias1,alias2:adjacency.xml,mpls.xml,pfe.xml\". ")
            ;    

    std::string query_file;
    bool dump_to_moped = false;
    int approx = 1;
    unsigned int link_failures = 0;
    verification.add_options()
            ("query,q", po::value<std::string>(&query_file),
            "A file containing valid queries over the input network.")
            ("link,l", po::value<unsigned int>(&link_failures), "Number of link-failures to model.")
            ("approximation,a", po::value<int>(&approx), "-1=under,0=exact,1=over")
            ("moped,m", po::bool_switch(&dump_to_moped), "Dump the constructed PDA in a MOPED format (expects a singleton query-file).")
            ;    
    
    opts.add(input);
    opts.add(output);
    opts.add(verification);

    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, opts), vm);
    po::notify(vm);

    if (vm.count("help")) {
        std::cout << opts << "\n";
        return 1;
    }
    
    std::stringstream dummy;
    std::ostream& warnings = no_parser_warnings ? dummy : std::cerr;

    ptrie::map<Router*> mapping;
    std::vector<std::unique_ptr < Router>> routers;
    if (junos_config.size() > 0)
        parse_junos(routers, mapping, junos_config, warnings);

    for (auto& r : routers) {
        r->pair_interfaces();
    }

    if (print_dot) {
        std::cout << "digraph network {\n";
        for (auto& r : routers) {
            r->print_dot(std::cout);
        }
        std::cout << "}" << std::endl;
    }

    if (dump_json) {
        std::cout << "{\n";
        for (size_t i = 0; i < routers.size(); ++i) {
            if (i != 0)
                std::cout << ",\n";
            std::cout << "\"" << routers[i]->name() << "\":";
            routers[i]->print_json(std::cout);
        }
        std::cout << "\n}\n";
    }
    
    Builder builder;
    builder.
    
    
    return 0;
}
