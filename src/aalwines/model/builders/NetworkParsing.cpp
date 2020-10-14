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

#include <aalwines/model/builders/JuniperBuilder.h>
#include <aalwines/model/builders/PRexBuilder.h>
#include <aalwines/model/builders/AalWiNesBuilder.h>
#include <iostream>

namespace aalwines {

    Network NetworkParsing::parse(bool no_warnings) {
        if(!json_file.empty() && (!prex_routing.empty() || !prex_topo.empty() || !junos_config.empty()))
        {
            std::cerr << "--input cannot be used with --junos or --topology or --routing." << std::endl;
            exit(-1);
        }

        if(!junos_config.empty() && (!prex_routing.empty() || !prex_topo.empty()))
        {
            std::cerr << "--junos cannot be used with --topology or --routing." << std::endl;
            exit(-1);
        }

        if(prex_routing.empty() != prex_topo.empty())
        {
            std::cerr << "Both --topology and --routing have to be non-empty." << std::endl;
            exit(-1);
        }

        if(junos_config.empty() && prex_routing.empty() && prex_topo.empty() && json_file.empty())
        {
            std::cerr << "Either a Junos configuration or a P-Rex configuration or an AalWiNes json configuration must be given." << std::endl;
            exit(-1);
        }

        if(skip_pfe && junos_config.empty())
        {
            std::cerr << "--skip-pfe is only avaliable for --junos configurations." << std::endl;
            exit(-1);
        }

        std::stringstream dummy;
        std::ostream& warnings = no_warnings ? dummy : std::cerr;

        parsing_stopwatch.start();
        auto network = junos_config.empty() ? (json_file.empty() ?
                       PRexBuilder::parse(prex_topo, prex_routing, warnings) :
                       AalWiNesBuilder::parse(json_file, warnings)) :
                       JuniperBuilder::parse(junos_config, warnings, skip_pfe);
        parsing_stopwatch.stop();

        return network;
    }

}
