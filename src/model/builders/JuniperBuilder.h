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
 *  Copyright Peter G. Jensen
 */

/* 
 * File:   JuniperParser.h
 * Author: Peter G. Jensen <root@petergjoel.dk>
 *
 * Created on August 16, 2019, 11:38 AM
 */

#ifndef JUNIPERBUILDER_H
#define JUNIPERBUILDER_H

#include "model/Router.h"
#include "model/Network.h"

#include <rapidxml.hpp>

#include <string>
#include <ptrie/ptrie_map.h>
#include <ostream>
#include <memory>

namespace mpls2pda {
    class JuniperBuilder {
    public:
        static Network parse(
                     const std::string& network,
                     std::ostream& warnings, bool skip_pfe);
    private:
        static void router_parse_adjacency(Router& router, std::istream& data, std::vector<std::unique_ptr<Router>>&routers, ptrie::map<Router*>& mapping, 
                std::vector<const Interface*>&, std::ostream& warnings, std::unordered_map<const Interface*, uint32_t>& ipmap);
        static void router_parse_routing(Router& router, std::istream& data, std::istream& indirect, std::vector<const Interface*>&, std::ostream& warnings, bool skip_pfe);
        static RoutingTable parse_table(rapidxml::xml_node<char>* node, ptrie::map<std::pair<std::string, std::string>>&indirect, Router* parent, std::vector<const Interface*>& , std::ostream& warnings, bool skip_pfe);
        static Interface* parse_via(Router* parent, rapidxml::xml_node<char>* via, std::vector<const Interface*>&);
        static int parse_weight(rapidxml::xml_node<char>* nh);
        static void parse_ops(RoutingTable::forward_t& f, std::string& opstr);

    };
}
#endif /* JUNIPERPARSER_H */

