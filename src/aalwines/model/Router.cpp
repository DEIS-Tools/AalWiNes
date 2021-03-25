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
        _tables.clear();
        _tables.reserve(router._tables.size());
        std::unordered_map<const RoutingTable*,RoutingTable*> table_mapping;
        for (const auto& table : router._tables) {
            auto new_table = _tables.emplace_back(std::make_unique<RoutingTable>(*table)).get();
            table_mapping.emplace(table.get(), new_table);
        }
        for (const auto& interface : router._interfaces) {
            assert(_interfaces.size() == interface->id());
            auto new_interface = _interfaces.emplace_back(std::make_unique<Interface>(*interface)).get();
            _interface_map[interface->get_name()] = new_interface;
            new_interface->_parent = this;
            new_interface->set_table(table_mapping[interface->table()]);
        }
        for (auto& table : _tables) {
            table->update_interfaces([this](const Interface* old) -> Interface* {
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

    std::pair<bool,Interface*> Router::insert_interface(const std::string& interface_name, std::vector<const Interface*>& all_interfaces, bool make_table) {
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
        if (make_table) {
            interface->set_table(this->emplace_table());
        }
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
            for(auto& e : i->table()->entries()) {
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
            for(auto& e : i->table()->entries()) {
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
    size_t Router::count_rules() const {
        return std::transform_reduce(_tables.begin(), _tables.end(), 0, std::plus<>(), [](const auto& table){ return table->count_rules(); });
    }
    size_t Router::count_entries() const {
        return std::transform_reduce(_tables.begin(), _tables.end(), 0, std::plus<>(), [](const auto& table){ return table->entries().size(); });
    }

    void Router::set_latitude_longitude(const std::string& latitude, const std::string& longitude) {
        _coordinate.emplace(std::stod(latitude), std::stod(longitude));
    }

    bool Router::check_sanity(std::ostream& error_stream) const {
        if (_names.empty()) {
            error_stream << "Router with index " << index() << " does not have a name." << std::endl;
            return false;
        }
        if (_interfaces.size() != _interface_map.size()) {
            error_stream << "In router " << name() << " _interfaces.size() != _interface_map.size()." << std::endl;
            return false;
        }
        size_t interface_i = 0;
        for (const auto& interface : _interfaces) {
            if (interface->id() != interface_i) {
                error_stream << "Interface in router " << name() << " with id() " << interface->id() << " is in position " << interface_i << " of _interfaces. This is incorrect." << std::endl;
                return false;
            }
            if (interface->source() != this) {
                error_stream << "Interface " << interface->get_name() << " in router " << name() << " has source() != this." << std::endl;
                return false;
            }
            if (interface->match() == nullptr) {
                error_stream << "Interface " << interface->get_name() << " in router " << name() << " has match() == nullptr." << std::endl;
                return false;
            }
            if (interface->target() == nullptr) {
                error_stream << "Interface " << interface->get_name() << " in router " << name() << " has target() == nullptr." << std::endl;
                return false;
            }
            if (interface->match()->source() != interface->target()) {
                error_stream << "Interface " << interface->get_name() << " in router " << name() << " has match()->source() != target()." << std::endl;
                return false;
            }
            if (interface->table() == nullptr) {
                error_stream << "Interface " << interface->get_name() << " in router " << name() << " has match()->source() != target()." << std::endl;
                return false;
            }
            if (std::find_if(_tables.begin(), _tables.end(), [&interface](const auto& t){ return t.get() == interface->table(); }) == _tables.end()) {
                error_stream << "Interface " << interface->get_name() << " in router " << name() << " has table() which is not in _tables." << std::endl;
                return false;
            }
            interface_i++;
        }
        for (const auto& table : _tables) {
            for (const auto& entry : table->entries()) {
                for (const auto& forward : entry._rules) {
                    if (std::find_if(_interfaces.begin(), _interfaces.end(), [&forward](const auto& i){ return i.get() == forward._via;}) == _interfaces.end()) {
                        error_stream << "Forwarding rule on router " << name() << " uses _via interface that is not in _interfaces." << std::endl;
                        return false;
                    }
                }
            }
        }
        return true;
    }

    void Router::pre_process(std::ostream& log) {
        for (const auto& table : _tables) {
            table->pre_process_rules(log);
        }
    }

}
