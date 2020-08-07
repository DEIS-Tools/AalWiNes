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
        Network(routermap_t&& mapping, std::vector<std::unique_ptr<Router> >&& routers, std::vector<const Interface*>&& all_interfaces)
        : _mapping(std::move(mapping)), _routers(std::move(routers)), _all_interfaces(std::move(all_interfaces)) {}

        //void add_router();

        Router *get_router(size_t id);
        [[nodiscard]] const std::vector<std::unique_ptr<Router>>& routers() const { return _routers; }

        void inject_network(Interface* link, Network&& nested_network, Interface* nested_ingoing,
                Interface* nested_outgoing, RoutingTable::label_t pre_label, RoutingTable::label_t post_label);
        void concat_network(Interface *link, Network &&nested_network, Interface *nested_ingoing, RoutingTable::label_t post_label);
        [[nodiscard]] size_t size() const { return _routers.size(); }

        std::unordered_set<Query::label_t> interfaces(filter_t& filter);
        [[nodiscard]] const std::vector<const Interface*>& all_interfaces() const { return _all_interfaces; }

        void print_dot(std::ostream& s);
        void print_simple(std::ostream& s);
        void print_json(std::ostream& s);

        // TODO: move these functions out:
        void write_prex_topology(std::ostream& s);
        void write_prex_routing(std::ostream& s);


        static Network make_network(const std::vector<std::string>& names, const std::vector<std::vector<std::string>>& links);

        std::string name;

    private:
        // NO TOUCHEE AFTER INIT!
        routermap_t _mapping;
        std::vector<std::unique_ptr<Router>> _routers;
        std::vector<const Interface*> _all_interfaces;
        void move_network(Network&& nested_network);
    };
}

#endif /* NETWORK_H */

