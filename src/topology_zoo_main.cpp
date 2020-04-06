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
    output.add_options()
            ("write-topology,t", po::value<std::string>(&topology_destination), "Write the topology in the P-Rex format to the given file.")
            ("write-routing,r", po::value<std::string>(&routing_destination), "Write the Routing in the P-Rex format to the given file.")
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
    if (topology_destination.empty() && routing_destination.empty() && dot_pos > 0 && topo_zoo.substr(dot_pos + 1) =="gml") {
        const auto name = topo_zoo.substr(0, dot_pos);
        topology_destination = name + "-topo.xml";
        routing_destination = name + "-routing.xml";
    } else if (topology_destination.empty() || routing_destination.empty()) {
        std::cerr << "Please provide routing and topology output file" << std::endl;
        exit(-1);
    }

    //std::ostream& warnings = std::cerr; // TODO: Consider implementing silent version
    auto network = TopologyZooBuilder::parse(topo_zoo); //, warnings);


    // TODO: Construct routes on network!


    std::ofstream out_topo(topology_destination);
    if(out_topo.is_open()) {
        network.write_prex_topology(out_topo);
    } else {
        std::cerr << "Could not open --write-topology\"" << topology_destination << "\" for writing" << std::endl;
        exit(-1);
    }
    std::ofstream out_route(routing_destination);
    if(out_route.is_open()) {
        network.write_prex_routing(out_route);
    } else {
        std::cerr << "Could not open --write-routing\"" << routing_destination << "\" for writing" << std::endl;
        exit(-1);
    }

}
