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
 *  Copyright Dan Kristiansen
 */

/*
 * File:   TopologyBuilder.h
 * Author: Dan Kristiansen
 *
 * Created on 04-06-2020.
 */

#ifndef AALWINES_TOPOLOGYBUILDER_H
#define AALWINES_TOPOLOGYBUILDER_H

#include <aalwines/model/Network.h>
#include <aalwines/model/builders/AalWiNesBuilder.h>
#include <utility>

namespace aalwines {

    class TopologyBuilder {
    public:

        // Parses a network topology in the gml format used by Topology Zoo.
        static Network parse(const std::string& gml, std::ostream& warnings = std::cerr);

        // Extracts the topology (assuming all links are bidirectional) from the network into the json format.
        // Note: The standard to_json function uses non-empty routing-tables to determine existence of links (and direction).
        static json json_topology(const aalwines::Network& network, bool no_routing = true);

    };
}


#endif //AALWINES_TOPOLOGYBUILDER_H