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
#include <aalwines/utils/json_stream.h>

#include <ptrie/ptrie_map.h>
#include <utility>
#include <vector>
#include <memory>
#include <functional>
#include <sstream>

namespace aalwines {
    class Network {
    public:
        using routermap_t = string_map<Router*>;

        Network(routermap_t&& mapping, std::vector<std::unique_ptr<Router> >&& routers, std::vector<const Interface*>&& all_interfaces)
        : _mapping(std::move(mapping)), _routers(std::move(routers)), _all_interfaces(std::move(all_interfaces)) {};

        Network() = default;
        explicit Network(std::string name) : name(std::move(name)) {};

        virtual ~Network() = default;
        Network(const Network& network) {
            *this = network;
        };
        Network& operator=(const Network& other);
        Network(Network&&) = default;
        Network& operator=(Network&&) = default;

        template<typename... Args >
        Router* add_router(std::string router_name, Args&&... args) {
            return add_router(std::vector<std::string>{std::move(router_name)}, std::forward<Args>(args)...);
        }
        template<typename... Args >
        Router* add_router(std::vector<std::string> names, Args&&... args) {
            auto id = _routers.size();
            _routers.emplace_back(std::make_unique<Router>(id, names, std::forward<Args>(args)...));
            auto router = _routers.back().get();
            for (const auto& router_name : names) {
                auto res = _mapping.insert(router_name);
                if (!res.first) {
                    std::stringstream es;
                    es << "error: Duplicate definition of \"" << router_name << "\", previously found in entry " << _mapping.get_data(res.second)->index() << std::endl;
                    throw base_error(es.str());
                }
                _mapping.get_data(res.second) = router;
            }
            return router;
        }
        Router* get_router(size_t id);
        Router* find_router(const std::string& router_name);
        [[nodiscard]] const std::vector<std::unique_ptr<Router>>& routers() const { return _routers; }
        [[nodiscard]] size_t size() const { return _routers.size(); }

        std::pair<bool, Interface*> insert_interface_to(const std::string& interface_name, Router* router);
        std::pair<bool, Interface*> insert_interface_to(const std::string& interface_name, const std::string& router_name);
        [[nodiscard]] const std::vector<const Interface*>& all_interfaces() const { return _all_interfaces; }
        std::unordered_set<Query::label_t> interfaces(filter_t& filter);

        void add_null_router();

        void inject_network(Interface* link, Network&& nested_network, Interface* nested_ingoing,
                            Interface* nested_outgoing, RoutingTable::label_t pre_label, RoutingTable::label_t post_label);
        void concat_network(Interface *link, Network &&nested_network, Interface *nested_ingoing, RoutingTable::label_t post_label);

        static Network make_network(const std::vector<std::string>& names, const std::vector<std::vector<std::string>>& links);
        static Network make_network(const std::vector<std::pair<std::string,Coordinate>>& names, const std::vector<std::vector<std::string>>& links);
        void print_dot(std::ostream& s) const;
        void print_simple(std::ostream& s) const;
        void print_json(json_stream& json_output) const;

        std::string name;

    private:
        routermap_t _mapping;
        std::vector<std::unique_ptr<Router>> _routers;
        std::vector<const Interface*> _all_interfaces;

        void move_network(Network&& nested_network);
    };
}

#endif /* NETWORK_H */

