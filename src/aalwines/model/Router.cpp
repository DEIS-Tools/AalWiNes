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
 * File:   Router.cpp
 * Author: Peter G. Jensen <root@petergjoel.dk>
 * 
 * Created on June 24, 2019, 11:22 AM
 */

#include "Router.h"
#include "Network.h"
#include "aalwines/utils/errors.h"

#include <vector>
#include <streambuf>
#include <sstream>
#include <set>
#include <cassert>

namespace aalwines {

    Router& Router::operator=(const Router& router) {
        _index = router._index;
        _names = router._names;
        _coordinate = router._coordinate;
        _is_null = router._is_null;
        _interfaces.clear();
        _interfaces.reserve(router._interfaces.size());
        // _interface_map = router._interface_map; // Copy of ptrie not working.
        _interface_map = string_map<Interface*>(); // Start from empty map instead

        for (auto& interface : router._interfaces) {
            assert(_interfaces.size() == interface->id());
            auto new_interface = _interfaces.emplace_back(std::make_unique<Interface>(*interface)).get();
            _interface_map[interface->get_name()] = new_interface;
            new_interface->_parent = this;
        }
        for (auto& interface : _interfaces) {
            interface->table().update_interfaces([this](const Interface* old) -> Interface* {
                return old == nullptr ? nullptr : this->_interfaces[old->id()].get();
            });
        }
        return *this;
    }

    void Router::add_name(const std::string& name) {
        _names.emplace_back(name);
    }

    void Router::change_name(const std::string& name) {
        assert(_names.size() == 1);
        _names.clear();
        _names.emplace_back(name);
    }

    const std::string& Router::name() const {
        assert(!_names.empty());
        return _names.back();
    }

    std::pair<bool,Interface*> Router::insert_interface(const std::string& interface_name, std::vector<const Interface*>& all_interfaces) {
        auto res = _interface_map.insert(interface_name);
        if (!res.first) {
            return std::make_pair(false, _interface_map.get_data(res.second));
        }
        auto iid = _interfaces.size();
        auto gid = all_interfaces.size();
        _interfaces.emplace_back(std::make_unique<Interface>(iid, gid, this));
        auto interface = _interfaces.back().get();
        all_interfaces.emplace_back(interface);
        _interface_map.get_data(res.second) = interface;
        return std::make_pair(true, interface);
    }

    Interface* Router::get_interface(const std::string& interface_name, std::vector<const Interface*>& all_interfaces) {
        return insert_interface(interface_name, all_interfaces).second;
    }

    Interface* Router::find_interface(const std::string& interface_name) {
        auto res = _interface_map.exists(interface_name);
        return res.first ? _interface_map.get_data(res.second) : nullptr;
    }

    void Interface::make_pairing(Interface* interface) {
        _matching = interface;
        interface->_matching = this;
        interface->_target = _parent;
        _target = interface->_parent;
    }

    std::string Interface::get_name() const {
        return _parent == nullptr ? "" : _parent->interface_name(_id);
    }

    std::string Router::interface_name(size_t i) const {
        return _interface_map.at(i);
    }

    void Router::print_dot(std::ostream& out) const {
        if (_interfaces.empty()) return;
        std::string n;
        for (auto& i : _interfaces) {
            n = interface_name(i->id());
            auto tgtstring = i->target() != nullptr ? i->target()->name() : "SINK";
            out << "\"" << name() << "\" -> \"" << tgtstring
                    << "\" [ label=\"" << n << "\" ];\n";
        }
        if (_interfaces.empty()) {
            out << "\"" << name() << "\" [shape=triangle];\n";
        } else {
            out << "\"" << name() << "\" [shape=circle];\n";
        }
        out << "\n";
    }

    void Router::print_simple(std::ostream& s) const {
        for(auto& i : _interfaces) {
            auto name = interface_name(i->id());
            s << "\tinterface: \"" << name << "\"\n";
            const RoutingTable& table = i->table();
            for(auto& e : table.entries()) {
                s << "\t\t[" << e._top_label << "] {\n";
                for(auto& fwd : e._rules) {
                    s << "\t\t\t" << fwd._priority << " |-[";
                    for(auto& o : fwd._ops) {
                        o.print_json(s, false, false);
                    }
                    s << "]-> ";
                    auto via = fwd._via;
                    if(via) {
                        s << via->source()->name() << ".";
                        auto tn = fwd._via->source()->interface_name(fwd._via->id());
                        s << tn << "\n";
                    } else {
                        s << "NULL\n";
                    }
                }
                s << "\t\t}\n";
            }
        }
    }

    void Router::print_json(json_stream& json_output) const {
        if (_coordinate) {
            json_output.entry("lat", _coordinate->latitude());
            json_output.entry("lng", _coordinate->longitude());
        }
        std::unordered_map<std::string,std::unordered_set<Query::label_t>> interfaces;
        std::set<std::string> targets;
        std::string if_name;
        for(auto& i : _interfaces) {
            if_name = interface_name(i->id());
            auto& label_set = interfaces.try_emplace(if_name).first->second;

            const RoutingTable& table = i->table();
            for(auto& e : table.entries()) {
                label_set.emplace(e._top_label);
                for(auto& fwd : e._rules) {
                    auto via = fwd._via;
                    if (via) {
                        auto tn = via->target()->name();
                        targets.insert(tn);
                    }
                }
            }
        }
        auto json_targets = json::array();
        for(auto& tn : targets) {
            json_targets.push_back(tn);
        }
        json_output.entry("targets", json_targets);

        json_output.begin_object("interfaces");
        for(const auto& in : interfaces) {
            auto labels = json::array();
            for (const auto& label : in.second) {
                std::stringstream s;
                s << label;
                labels.push_back(s.str());
            }
            json_output.entry(in.first, labels);
        }
        json_output.end_object();
    }

    void Router::set_latitude_longitude(const std::string& latitude, const std::string& longitude) {
        _coordinate.emplace(std::stod(latitude), std::stod(longitude));
    }
}
