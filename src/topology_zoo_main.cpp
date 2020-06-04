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
 *  Copyright Peter G. Jensen and Morten K. Schou
 */

/*
 * File:   topology_zoo_main.cpp
 * Author: Morten K. Schou <morten@h-schou.dk>
 *
 * Created on 06-04-2020
 */

#include <aalwines/model/builders/TopologyZooBuilder.h>
#include <boost/program_options.hpp>
#include <iostream>
#include <string>
#include <fstream>
#include <aalwines/synthesis/RouteConstruction.h>

namespace po = boost::program_options;
using namespace aalwines;

int main(int argc, const char** argv)
{
    po::options_description opts;
    opts.add_options()
            ("help,h", "produce help message");
    po::options_description output("Output Options");
    po::options_description input("Input Options");

    std::string topo_zoo;
    input.add_options()
            ("zoo,z", po::value<std::string>(&topo_zoo),"A gml-file defining the topology in the format from topology zoo")
            ;
    std::string topology_destination;
    std::string routing_destination;
    std::string query_destination;
    output.add_options()
            ("write-topology,t", po::value<std::string>(&topology_destination), "Write the topology in the P-Rex format to the given file.")
            ("write-routing,r", po::value<std::string>(&routing_destination), "Write the Routing in the P-Rex format to the given file.")
            ("write-query,q", po::value<std::string>(&query_destination), "Write a query with description to the given file.")
            ;
    opts.add(input);
    opts.add(output);

    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, opts), vm);
    po::notify(vm);
    if (vm.count("help")) {
        std::cout << opts << "\n";
        return 1;
    }
    if(topo_zoo.empty()) {
        std::cerr << "Please provide topology zoo input file" << std::endl;
        exit(-1);
    }
    size_t dot_pos = topo_zoo.find_last_of('.');
    if (topology_destination.empty() && routing_destination.empty() && query_destination.empty() && dot_pos > 0 && topo_zoo.substr(dot_pos + 1) =="gml") {
        const auto name = topo_zoo.substr(0, dot_pos);
        topology_destination = name + "-topo.xml";
        routing_destination = name + "-routing.xml";
        query_destination = name + "-queries.json";
    } else if (topology_destination.empty() || routing_destination.empty()) {
        std::cerr << "Please provide routing and topology output file" << std::endl;
        exit(-1);
    }

    //std::ostream& warnings = std::cerr; // TODO: Consider implementing silent version
    auto network = TopologyZooBuilder::parse(topo_zoo); //, warnings);
    std::vector<std::pair<Router*, Router*>> unlinked_routers;

    for (auto &r : network.get_all_routers()) {
        if (r->is_null()) continue;
        if (!r->coordinate() || (r->coordinate().value().latitude() == 0 && r->coordinate().value().longitude() == 0)) {
            // Try to find a good coordinate.
            std::vector<Coordinate> neighbours;
            for (auto &i: r->interfaces()) {
                if (!i->target()->is_null() && i->target()->coordinate()
                && (i->target()->coordinate().value().latitude() != 0 || i->target()->coordinate().value().longitude() != 0)) {
                    neighbours.push_back(i->target()->coordinate().value());
                }
            }
            if (neighbours.empty()) {
                std::cerr << "Warning: No neighbour with coordinate for router " << r->name() << "." << std::endl;
            } else {
                r->set_coordinate(Coordinate::mid_point(neighbours));
            }
        }
    }


    uint64_t i = 42;
    auto next_label = [&i](){return Query::label_t(Query::type_t::MPLS, 0, i++);};
    auto cost = [](const Interface* interface){ return interface->source()->coordinate() && interface->target()->coordinate() ?
                interface->source()->coordinate()->distance_to(interface->target()->coordinate().value()) : 1 ; };

    for(auto &r : network.get_all_routers()){
        if(r->is_null()) continue;
        for(auto &r_p : network.get_all_routers()){
            if (r == r_p || r_p->is_null()) continue;
            auto success = RouteConstruction::make_data_flow(r->get_null_interface(), r_p->get_null_interface(), next_label, cost);
            if(!success){
                unlinked_routers.emplace_back(std::make_pair(r.get(), r_p.get()));
            }
        }
    }
    for(auto& inf : network.all_interfaces()){
        if (inf->target()->is_null() || inf->source()->is_null()) continue;
        RouteConstruction::make_reroute(inf, next_label, cost);
    }

    std::ofstream out_topo(topology_destination);
    if(out_topo.is_open()) {
        network.write_prex_topology(out_topo);
    } else {
        std::cerr << "Could not open --write-topology \"" << topology_destination << "\" for writing" << std::endl;
        exit(-1);
    }
    std::ofstream out_route(routing_destination);
    if(out_route.is_open()) {
        network.write_prex_routing(out_route);
    } else {
        std::cerr << "Could not open --write-routing \"" << routing_destination << "\" for writing" << std::endl;
        exit(-1);
    }
    if (!query_destination.empty()) {
        std::ofstream out_query(query_destination);
        if(out_query.is_open()) {
            out_query << "[\n"
                      << "    {\n"
                      << R"(        "Description": "Find a trace entering the network at )" << network.get_router(0)->name() << " and leaving the network at " << network.get_router(1)->name() <<  " with no failed links.\",\n"
                      << R"(        "Query": ")"
                      << "<ip> [.#" << network.get_router(0)->name() << "] .* [" << network.get_router(1)->name() << "#.] <ip> 0 DUAL\"\n"
                      << "    },\n"
                      << "    {\n"
                      << R"(        "Description": "Find a trace entering )" << network.get_router(0)->name() << " with one MPLS label and leaving " << network.get_router(1)->name() <<  " least one MPLS label, where at most 1 link is failed.\",\n"
                      << R"(        "Query": ")"
                      << "<smpls ip> [.#" << network.get_router(0)->name() << "] .* [" << network.get_router(1)->name() << "#.] <mpls* smpls ip> 1 DUAL\"\n"
                      << "    }\n"
                      << "]";
        } else {
            std::cerr << "Could not open --write-query \"" << query_destination << "\" for writing" << std::endl;
            exit(-1);
        }
    }
}
