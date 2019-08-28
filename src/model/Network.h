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
 * File:   Network.h
 * Author: Peter G. Jensen <root@petergjoel.dk>
 *
 * Created on July 17, 2019, 2:17 PM
 */

#ifndef NETWORK_H
#define NETWORK_H
#include "model/Router.h"
#include "model/RoutingTable.h"
#include "Query.h"
#include "filter.h"

#include <ptrie/ptrie_map.h>
#include <vector>
#include <memory>
#include <functional>

namespace mpls2pda {
class Network {
public:
    Network(ptrie::map<Router*>&& mapping, std::vector<std::unique_ptr < Router>>&& routers, std::vector<const Interface*>&& all_interfaces);
    Network(const Network&) = default;
    Network(Network&&) = default;
    Network& operator=(const Network&) = default;
    Network& operator=(Network&&) = default;
    
    const Router* get_router(size_t id) const;
    size_t size() const { return _routers.size(); }
    std::unordered_set<Query::label_t> interfaces(filter_t& filter);
    std::unordered_set<Query::label_t> get_labels(uint64_t label, uint64_t mask, Query::type_t type);
    std::unordered_set<Query::label_t> all_labels();
    const std::vector<const Interface*>& all_interfaces() const { return _all_interfaces; }
    void print_dot(std::ostream& s);
private:
    // NO TOUCHEE AFTER INIT!
    ptrie::map<Router*> _mapping;
    std::vector<std::unique_ptr<Router>> _routers;    
    std::unordered_map<Query::label_t,std::vector<std::pair<const RoutingTable::entry_t*, const Router*>>> _label_map;
    std::vector<const Interface*> _all_interfaces;
    std::unordered_set<Query::label_t> _label_cache;
    

};
}

#endif /* NETWORK_H */

