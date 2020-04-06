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
#include "Router.h"
#include "RoutingTable.h"
#include "Query.h"
#include "filter.h"

#include <ptrie/ptrie_map.h>
#include <vector>
#include <memory>
#include <functional>

namespace aalwines {
class Network {
public:
    using routermap_t = ptrie::map<char, Router*>;
    Network(routermap_t&& mapping, std::vector<std::unique_ptr < Router>>&& routers, std::vector<const Interface*>&& all_interfaces);
    Network(const Network&) = default;
    Network(Network&&) = default;
    Network& operator=(const Network&) = default;
    Network& operator=(Network&&) = default;

    Router *get_router(size_t id);
    const std::vector<std::unique_ptr<Router>>& get_all_routers() const { return _routers; }

    void inject_network(Interface* link, Network&& nested_network, Interface* nested_ingoing,
            Interface* nested_outgoing, RoutingTable::label_t pre_label, RoutingTable::label_t post_label);
    void concat_network(Interface *link, Network &&nested_network, Interface *nested_ingoing, RoutingTable::label_t post_label);
    size_t size() const { return _routers.size(); }
    const routermap_t& get_mapping() const { return _mapping; }
    int get_max_label() const { return _max_label; }

    std::unordered_set<Query::label_t> interfaces(filter_t& filter);
    std::unordered_set<Query::label_t> get_labels(uint64_t label, uint64_t mask, Query::type_t type, bool exact = false);
    std::unordered_set<Query::label_t> all_labels();
    std::vector<const Interface*>& all_interfaces() { return _all_interfaces; }
    void print_dot(std::ostream& s);
    void print_simple(std::ostream& s);
    bool is_service_label(const Query::label_t&) const;
    void write_prex_topology(std::ostream& s);
    void write_prex_routing(std::ostream& s);
private:
    // NO TOUCHEE AFTER INIT!
    routermap_t _mapping;
    std::vector<std::unique_ptr<Router>> _routers;
    size_t _total_labels = 0;
    std::vector<const Interface*> _all_interfaces;
    std::unordered_set<Query::label_t> _label_cache;
    std::unordered_set<Query::label_t> _non_service_label;
    uint64_t _max_label = 0;
    void move_network(Network&& nested_network);
};
}

#endif /* NETWORK_H */

