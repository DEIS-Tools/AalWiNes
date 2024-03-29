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
 * File:   Network.cpp
 * Author: Peter G. Jensen <root@petergjoel.dk>
 * 
 * Created on July 17, 2019, 2:17 PM
 */

#include "Network.h"
#include "filter.h"

#include <cassert>
#include <map>

namespace aalwines {

    Network& Network::operator=(const Network& other) {
        if (this == &other) return *this; // Safely handle self-assignment.

        name = other.name;
        // _mapping = other._mapping; Copy of ptrie still not working.
        _mapping = routermap_t(); // Start from empty map instead.
        _routers.clear();
        _routers.reserve(other._routers.size());
        _all_interfaces.clear();
        _all_interfaces.reserve(other._all_interfaces.size());
        // Copy-construct routers
        for (const auto& router : other._routers) {
            auto router_p = _routers.emplace_back(std::make_unique<Router>(*router)).get();
            for (const auto& router_name : router->names()) {
                _mapping[router_name] = router_p;
            }
        }
        /*// Update pointers in _mapping // Do this when copy of ptrie works..
        for (auto& router : _mapping) {
            router = _routers[router->index()].get();
        }*/

        // Add interface pointers
        for (const auto& router : _routers) {
            for (const auto& interface : router->interfaces()) {
                _all_interfaces.push_back(interface.get());
            }
        }
        // Ensure global_id of interfaces is correct.
        std::sort(_all_interfaces.begin(), _all_interfaces.end(), [](const Interface* a, const Interface* b){
            return a->global_id() < b->global_id();
        });
#ifndef NDEBUG
        for(size_t i = 0; i < _all_interfaces.size(); ++i) {
            assert(_all_interfaces[i]->global_id() == i);
        }
#endif
        // Update pairings
        for (auto& router : _routers) {
            for (auto& interface : router->interfaces()) {
                auto match =  interface->match();
                if (match != nullptr) {
                    assert(match->source() != nullptr);
                    interface->make_pairing(_routers[match->source()->index()]->interfaces()[match->id()].get());
                }
            }
        }
        return *this;
    }

    Router* Network::find_router(const std::string& router_name) const {
        auto from_res = _mapping.exists(router_name);
        return from_res.first ? _mapping.get_data(from_res.second) : nullptr;
    }

    Router* Network::get_router(size_t id) {
        return id < _routers.size() ? _routers[id].get() : nullptr;
    }

    std::pair<bool, Interface*> Network::insert_interface_to(const std::string& interface_name, Router* router, bool make_table) {
        return router->insert_interface(interface_name, _all_interfaces, make_table);
    }
    std::pair<bool, Interface*> Network::insert_interface_to(const std::string& interface_name, const std::string& router_name, bool make_table) {
        auto router = find_router(router_name);
        return router == nullptr ? std::make_pair(false, nullptr) : insert_interface_to(interface_name, router, make_table);
    }

    void Network::add_null_router() {
        bool used = false;
        auto null_router = add_router("NULL", true);
        auto table = null_router->emplace_table();
        for(const auto& r : routers()) {
            for(const auto& inf : r->interfaces()) {
                if(inf->match() == nullptr) {
                    used = true;
                    std::stringstream interface_name;
                    interface_name << "i" << inf->global_id();
                    auto new_inf = insert_interface_to(interface_name.str(), null_router, false).second;
                    new_inf->make_pairing(inf.get());
                    new_inf->set_table(table);
                }
            }
        }
        table->sort();
        if (!used) {
            _routers.pop_back();
            _mapping.erase("NULL");
        }
    }

    std::unordered_set<size_t> Network::interfaces(filter_t& filter) {
        std::unordered_set<size_t> res;
        for (const auto& r : _routers) {
            if (filter._from(r->name())) {
                for (const auto& i : r->interfaces()) {
                    if (filter._link(r->interface_name(i->id()), i->match()->get_name(), i->target()->name())) {
                        res.insert(i->global_id());
                    }
                }
            }
        }
        return res;
    }

    void Network::move_network(Network&& nested_network) {
        // Find NULL router
        auto null_router = _mapping["NULL"];

        // Move old network into new network.
        for (auto&& e : nested_network._routers) {
            if (e->is_null()) {
                continue;
            }
            // Find unique name for router
            std::string new_name = e->name();
            while(_mapping.exists(new_name).first){
                new_name += "'";
            }
            e->change_name(new_name);

            // Add interfaces to _all_interfaces and update their global id.
            for (auto&& inf: e->interfaces()) {
                inf->set_global_id(_all_interfaces.size());
                _all_interfaces.push_back(inf.get());
                // Transfer links from old NULL router to new NULL router.
                if (inf->target()->is_null()){
                    std::stringstream ss;
                    ss << "i" << inf->global_id();
                    insert_interface_to(ss.str(), null_router).second->make_pairing(inf.get());
                }
            }

            // Move router to new network
            e->set_index(_routers.size());
            _routers.emplace_back(std::move(e));
            _mapping[new_name] = _routers.back().get();
        }
    }

    void Network::inject_network(Interface* link, Network&& nested_network, Interface* nested_ingoing,
            Interface* nested_outgoing, RoutingTable::label_t pre_label, RoutingTable::label_t post_label) {
        assert(nested_ingoing->target()->is_null());
        assert(nested_outgoing->target()->is_null());
        assert(this->size());
        assert(nested_network.size());

        // Pair interfaces for injection and create virtual interface to filter post_label before POP.
        auto link_end = link->match();
        link->make_pairing(nested_ingoing);
        auto virtual_guard = insert_interface_to("__virtual_guard__", nested_outgoing->source()).second; // Assumes these names are unique for this router.
        auto nested_end_link = insert_interface_to("__end_link__", nested_outgoing->source()).second;
        nested_outgoing->make_pairing(virtual_guard);
        link_end->make_pairing(nested_end_link);

        move_network(std::move(nested_network));

        // Add push and pop rules.
        for (auto&& table : link->source()->tables()) {
            table->add_to_outgoing(link, {RoutingTable::op_t::PUSH, pre_label});
        }
        virtual_guard->table()->add_rule(post_label, RoutingTable::action_t(RoutingTable::op_t::POP), nested_end_link);
    }

    void Network::concat_network(Interface* link, Network&& nested_network, Interface* nested_ingoing, RoutingTable::label_t post_label) {
        assert(nested_ingoing->target()->is_null());
        assert(link->target()->is_null());
        assert(this->size());
        assert(nested_network.size());

        move_network(std::move(nested_network));

        // Pair interfaces for concatenation.
        link->make_pairing(nested_ingoing);
    }

    void Network::print_dot(std::ostream& s) const {
        s << "digraph network {\n";
        for (const auto& r : _routers) {
            r->print_dot(s);
        }
        s << "}" << std::endl;
    }
    void Network::print_dot_topo(std::ostream& s) const {
        s << "graph network {\n";
        for (const auto& r : _routers) {
            if (r->is_null()) continue;
            bool is_connected = false;
            for (const auto& i : r->interfaces()) {
                if (!i->target() || i->target()->is_null() || !i->match()) continue;
                is_connected = true;
                // Only make one edge per connected interface pair
                if (i->match()->global_id() < i->global_id()) continue;
                s << "\"" << r->name() << "\" -- \"" << i->target()->name() << "\";\n";
            }
            if (!is_connected) {
                s << "\"" << r->name() << "\" [shape=triangle];\n";
            }
        }
        s << "}" << std::endl;
    }
    void Network::print_simple(std::ostream& s) const {
        for(const auto& r : _routers) {
            s << "router: \"" << r->name() << "\":\n";
            r->print_simple(s);
        }
    }
    void Network::print_json(json_stream& json_output) const {
        json_output.begin_object("routers");
        for(const auto& r : _routers) {
            if (r->is_null()) continue;
            json_output.begin_object(r->name());
            r->print_json(json_output);
            json_output.end_object();
        }
        json_output.end_object();
    }

    void Network::print_info(std::ostream& s) const {
        s << "Network: " << name << std::endl;
        s << "Routers: " << std::count_if(_routers.begin(), _routers.end(), [](const auto& r){ return !r->is_null(); }) << std::endl;
        s << "Entries: " << std::transform_reduce(_routers.begin(), _routers.end(), 0, std::plus<>(), [](const auto& r){ return r->count_entries(); }) << std::endl;
        s << "Rules: " << std::transform_reduce(_routers.begin(), _routers.end(), 0, std::plus<>(), [](const auto& r){ return r->count_rules(); }) << std::endl;
    }


    Network Network::make_network(const std::vector<std::string>& names, const std::vector<std::vector<std::string>>& links) {
        Network network;
        for (size_t i = 0; i < names.size(); ++i) {
            auto name = names[i];
            auto router = network.add_router(name);
            network.insert_interface_to("i" + name, router);
            for (const auto& other : links[i]) {
                network.insert_interface_to(other, router);
            }
        }
        for (size_t i = 0; i < names.size(); ++i) {
            auto name = names[i];
            for (const auto &other : links[i]) {
                auto router1 = network.find_router(name);
                assert(router1 != nullptr);
                auto router2 = network.find_router(other);
                if(router2 == nullptr) continue;
                router1->find_interface(other)->make_pairing(router2->find_interface(name));
            }
        }
        network.add_null_router();
        return network;
    }
    Network Network::make_network(const std::vector<std::pair<std::string,std::optional<Coordinate>>>& names, const std::vector<std::vector<std::string>>& links) {
        Network network;
        std::map<std::pair<std::string,std::string>, std::string> _interface_map;
        for (size_t i = 0; i < names.size(); ++i) {
            auto name = names[i].first;
            auto coordinate = names[i].second;
            auto router = network.add_router(name, coordinate);
            network.insert_interface_to("eg" + std::to_string(0), router);
            size_t interface_count = 0;
            for (const auto& other : links[i]) {
                network.insert_interface_to("in" + std::to_string(interface_count), router);
                _interface_map.insert({std::make_pair(name,other), ("in" + std::to_string(interface_count))});
                ++interface_count;
            }
        }
        for (size_t i = 0; i < names.size(); ++i) {
            auto name = names[i].first;
            for (const auto &other : links[i]) {
                auto router1 = network.find_router(name);
                assert(router1 != nullptr);
                auto router2 = network.find_router(other);
                if(router2 == nullptr) continue;
                router1->find_interface(_interface_map[std::make_pair(name,other)])->make_pairing(router2->find_interface(_interface_map[std::make_pair(other,name)]));
            }
        }
        network.add_null_router();
        return network;
    }

    bool Network::check_sanity(std::ostream& error_stream) const {
        size_t router_i = 0;
        for (const auto& router : _routers) {
            if (router->index() != router_i) {
                error_stream << "Router with index() " << router->index() << " are in position " << router_i << " of _routers. This is incorrect." << std::endl;
                return false;
            }
            if (!router->check_sanity(error_stream)) {
                error_stream << "When checking router with index " << router_i << " in the network." << std::endl;
                return false;
            }
            for (const auto& router_name : router->names()) {
                if (find_router(router_name) != router.get()) {
                    error_stream << "Router name " << router_name << " is not mapped to router with index " << router->index() << "." << std::endl;
                    return false;
                }
            }
            for (const auto& interface : router->interfaces()) {
                if (std::find_if(_all_interfaces.begin(), _all_interfaces.end(), [&interface](const auto& i){ return i == interface.get(); }) == _all_interfaces.end()) {
                    error_stream << "Interface " << interface->get_name() << " in router " << router->name() << " is not in _all_interfaces." << std::endl;
                    return false;
                }
            }
            router_i++;
        }
        size_t interface_i = 0;
        for (const Interface* interface : _all_interfaces) {
            if (interface == nullptr) {
                error_stream << "Network contains nullptr in _all_interfaces." << std::endl;
                return false;
            }
            if (interface->source() == nullptr) {
                error_stream << "Interface in _all_interfaces has source() == nullptr." << std::endl;
                return false;
            }
            auto r = std::find_if(_routers.begin(), _routers.end(), [interface](const auto& r){ return r.get() == interface->source(); });
            if (r == _routers.end()) {
                error_stream << "Interface in _all_interfaces has source() that is not in _routers." << std::endl;
                return false;
            }
            auto t = std::find_if(_routers.begin(), _routers.end(), [interface](const auto& r){ return r.get() == interface->target(); });
            if (t == _routers.end()) {
                error_stream << "Interface in _all_interfaces has target() that is not in _routers." << std::endl;
                return false;
            }
            auto i = std::find_if(r->get()->interfaces().begin(), r->get()->interfaces().end(), [interface](const auto& i){ return i.get() == interface; });
            if (i == r->get()->interfaces().end()) {
                error_stream << "Interface in _all_interfaces was not found in its source()->interfaces()." << std::endl;
                return false;
            }
            if (interface->global_id() != interface_i) {
                error_stream << "Interface with global_id() " << interface_i << " is in position " << interface_i << " of _all_interfaces. This is incorrect." << std::endl;
                return false;
            }
            interface_i++;
        }
        return true;
    }

    void Network::prepare_tables() {
        for (const auto& router : _routers) {
            for (const auto& table : router->tables()) {
                std::unordered_set<const Interface*> out_interfaces;
                for (const auto& entry : table->entries()) {
                    for (const auto& rule : entry._rules) {
                        out_interfaces.emplace(rule._via);
                    }
                }
                table->set_out_interfaces(out_interfaces);
                table->sort();
                table->sort_rules();
            }
        }
    }

    void Network::pre_process(std::ostream& log) {
        for (const auto& router : _routers) {
            router->pre_process(log);
        }
    }

}
