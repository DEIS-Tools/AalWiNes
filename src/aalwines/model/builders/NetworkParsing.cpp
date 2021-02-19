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
 * File:   NetworkParsing.cpp
 * Author: Morten K. Schou <morten@h-schou.dk>
 *
 * Created on 14-10-2020.
 */

#include "NetworkParsing.h"

#include <aalwines/model/builders/AalWiNesBuilder.h>
#include <aalwines/model/builders/TopologyBuilder.h>
#include <aalwines/model/builders/NetworkSAXHandler.h>
#include <iostream>

namespace aalwines {

    Network NetworkParsing::parse(bool no_warnings) {

        if(!json_file.empty() && !topo_zoo.empty()) {
            std::cerr << "--input cannot be used with --gml." << std::endl;
            exit(-1);
        }

        std::stringstream dummy;
        std::ostream& warnings = no_warnings ? dummy : std::cerr;

        auto format = msgpack ? json::input_format_t::msgpack : json::input_format_t::json;

        parsing_stopwatch.start();
        auto network = !topo_zoo.empty()
                     ? TopologyBuilder::parse(topo_zoo, warnings)
                     : ((json_file.empty() || json_file == "-")
                        ? FastJsonBuilder::parse(std::cin, warnings, format)
                        : FastJsonBuilder::parse(json_file, warnings, format));
        parsing_stopwatch.stop();

        return network;
    }

}
