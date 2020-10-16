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
 * File:   NetworkParsing.h
 * Author: Morten K. Schou <morten@h-schou.dk>
 *
 * Created on 14-10-2020.
 */

#ifndef AALWINES_NETWORKPARSING_H
#define AALWINES_NETWORKPARSING_H

#include <aalwines/utils/stopwatch.h>
#include <aalwines/model/Network.h>

#include <string>

#include <boost/program_options.hpp>
namespace po = boost::program_options;

namespace aalwines {
    /**
     * NetworkParsing class handles CLI parameters for network parsing and performs the parsing based on these input options.
     */
    class NetworkParsing {
    public:
        explicit NetworkParsing(const std::string& caption = "Input Options") : input{caption} {
            input.add_options()
                ("input", po::value<std::string>(&json_file),
                 "An json-file defining the network in the AalWiNes MPLS Network format")
                ("juniper", po::value<std::string>(&junos_config),
                 "A file containing a network-description; each line is a router in the format \"name,alias1,alias2:adjacency.xml,mpls.xml,pfe.xml\". ")
                ("topology", po::value<std::string>(&prex_topo),
                 "An xml-file defining the topology in the P-Rex format")
                ("routing", po::value<std::string>(&prex_routing),
                 "An xml-file defining the routing in the P-Rex format")
                 ("gml", po::value<std::string>(&topo_zoo),"A gml-file defining the topology in the format from topology zoo")
                ("skip-pfe", po::bool_switch(&skip_pfe),
                 "Skip \"indirect\" cases of juniper-routing as package-drops (compatability with P-Rex semantics).")
                ;
        }

        [[nodiscard]] const po::options_description& options() const { return input; }
        [[nodiscard]] double duration() const { return parsing_stopwatch.duration(); }
        Network parse(bool no_warnings = false);

    private:
        std::string json_file, junos_config, prex_topo, prex_routing, topo_zoo;
        bool skip_pfe = false;
        po::options_description input;
        stopwatch parsing_stopwatch{false};
    };
}

#endif //AALWINES_NETWORKPARSING_H
