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
 *  Copyright Peter G. Jensen, all rights reserved.
 */

/* 
 * File:   PRexBuilder.cpp
 * Author: Peter G. Jensen <root@petergjoel.dk>
 * 
 * Created on August 16, 2019, 2:44 PM
 */

#include "PRexBuilder.h"

namespace mpls2pda {
    Network PRexBuilder::parse(const std::string& topo_fn, const std::string& routing_fn)
    {
        std::vector<std::unique_ptr<Router> > routers;
        std::vector<const Interface*> interfaces;
        ptrie::map<Router*> mapping;        
        return Network(std::move(mapping), std::move(routers), std::move(interfaces));
    }
}

