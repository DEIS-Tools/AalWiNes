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

namespace aalwines {

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


    Interface* Router::add_interface(const std::string& interface_name, std::vector<const Interface*>& all_interfaces) {
        auto res = _interface_map.insert(interface_name);
        if (!res.first) {
            return _interface_map.get_data(res.second);
        }
        auto iid = _interfaces.size();
        auto gid = all_interfaces.size();
        _interfaces.emplace_back(std::make_unique<Interface>(iid, gid, this));
        auto interface = _interfaces.back().get();
        all_interfaces.emplace_back(interface);
        _interface_map.get_data(res.second) = interface;
        return interface;
    }

    Interface* Router::find_interface(const std::string& interface_name) {
        auto res = _interface_map.exists(interface_name);
        return res.first ? _interface_map.get_data(res.second) : nullptr;
    }

    // TODO: Merge this with add_interface
    Interface* Router::get_interface(std::vector<const Interface*>& all_interfaces, const std::string& interface_name, Router* expected) {
        auto res = _interface_map.insert(interface_name);
        if (expected != nullptr && !res.first && _interface_map.get_data(res.second)->target() != expected) {
            auto tgt = _interface_map.get_data(res.second)->target();
            auto tname = tgt != nullptr ? tgt->name() : "SINK";
            std::stringstream e;
            e << "duplicate interface declaration, got " << tname
                    << " new target " << expected->name() << " on adjacency of " << name() << std::endl;
            throw base_error(e.str());
        }
        if (!res.first)
            return _interface_map.get_data(res.second);
        auto iid = _interfaces.size();
        auto gid = all_interfaces.size();
        _interfaces.emplace_back(std::make_unique<Interface>(iid, gid, expected, this));
        auto interface = _interfaces.back().get();
        all_interfaces.emplace_back(interface);
        _interface_map.get_data(res.second) = interface;
        return interface;
    }


    void Interface::make_pairing(Interface* interface) {
        _matching = interface;
        interface->_matching = this;
        interface->_target = _parent;
        _target = interface->_parent;
    }

    void Interface::make_pairing(std::vector<const Interface*>& all_interfaces, std::function<bool(const Interface*, const Interface*)> matcher) {
        if (_matching != nullptr)
            return;
        if (_target == nullptr)
            return;
        if (_target == _parent)
        {
            _matching = this;
            return;
        }
        _matching = nullptr;
        for (auto& i : _target->_interfaces) {
            if(matcher(this, i.get()))
            {
                if(_matching != nullptr)
                {
                    std::stringstream e;
                    e << "Non-unique paring of links between " << _parent->name() << " and " << _target->name() << "\n";
                    auto n = _parent->interface_name(_id);
                    auto n2 = _target->interface_name(_matching->_id);
                    auto n3 = _target->interface_name(i->_id);
                    e << n << " could be matched with both :\n";
                    e << n2 << " and\n";
                    e << n3 << std::endl;
                    throw base_error(e.str());
                }
                _matching = i.get();
            }            
        }


        if (_matching) {
            _matching->_matching = this;
        }
        else {

            std::stringstream e;
            auto n = _parent->interface_name(_id);
            e << _parent->name() << "." << n;
            auto iface = _target->get_interface(all_interfaces, e.str(), _parent);
            _matching = iface;
            iface->_matching = this;
        }
    }

    std::string Interface::get_name() const {
        return _parent == nullptr ? "" : _parent->interface_name(_id);
    }

    std::string Router::interface_name(size_t i) {
        return _interface_map.at(i);
    }

    void Router::pair_interfaces(std::vector<const Interface*>& interfaces, std::function<bool(const Interface*, const Interface*)> matcher) {
        for (auto& i : _interfaces)
            i->make_pairing(interfaces, matcher);
    }

    void Router::print_dot(std::ostream& out) {
        if (_interfaces.empty())
            return;
        std::string n;
        for (auto& i : _interfaces) {
            n = interface_name(i->id());
            auto tgtstring = i->target() != nullptr ? i->target()->name() : "SINK";
            out << "\"" << name() << "\" -> \"" << tgtstring
                    << "\" [ label=\"" << n << "\" ];\n";
        }
        if (_interfaces.empty())
            out << "\"" << name() << "\" [shape=triangle];\n";
        else
            out << "\"" << name() << "\" [shape=circle];\n";

        out << "\n";

    }

    void Router::print_simple(std::ostream& s) {
        for(auto& i : _interfaces)
        {
            auto name = interface_name(i->id());
            s << "\tinterface: \"" << name << "\"\n";
            const RoutingTable& table = i->table();
            for(auto& e : table.entries())
            {
                s << "\t\t[" << e._top_label << "] {\n";
                for(auto& fwd : e._rules)
                {
                    s << "\t\t\t" << fwd._priority << " |-[";
                    for(auto& o : fwd._ops)
                    {
                        o.print_json(s, false, false);
                    }
                    s << "]-> ";
                    auto via = fwd._via;
                    if(via)
                    {
                        s << via->source()->name() << ".";
                        auto tn = fwd._via->source()->interface_name(fwd._via->id());
                        s << tn << "\n";
                    }
                    else
                    {
                        s << "NULL\n";
                    }
                }
                s << "\t\t}\n";
            }
        }
    }

    void Router::print_json(std::ostream& s) {
        if (_coordinate) {
            s << "\t\t\t\"lat\": " << _coordinate->latitude() << ",\n\t\t\t\"lng\": " << _coordinate->longitude() << ",\n";
        }
        std::unordered_map<std::string,std::unordered_set<Query::label_t>> interfaces;
        std::set<std::string> targets;
        std::string if_name;
        for(auto& i : _interfaces)
        {
            if_name = interface_name(i->id());
            auto& label_set = interfaces.try_emplace(if_name).first->second;

            const RoutingTable& table = i->table();
            for(auto& e : table.entries())
            {
                label_set.emplace(e._top_label);
                for(auto& fwd : e._rules)
                {
                    auto via = fwd._via;
                    if (via)
                    {
                        auto tn = via->target()->name();
                        targets.insert(tn);
                    }
                }
            }
        }
        s << "\t\t\t\"targets\": [\n";
        bool first = true;
        for(auto& tn : targets)
        {
            if (first)
            {
                first = false;
            }
            else
            {
                s << ",\n";
            }
            s << "\t\t\t\t\"" << tn << "\"";
        }
        s << "\n\t\t\t],\n";

        s << "\t\t\t\"interfaces\": {\n";
        first = true;
        for(const auto& in : interfaces)
        {
            if (first)
            {
                first = false;
            }
            else
            {
                s << ",\n";
            }
            s << "\t\t\t\t\"" << in.first << "\": [";
            bool first_label = true;
            for (const auto& label : in.second) {
                if (first_label) {
                    first_label = false;
                } else {
                    s << ",";
                }
                s << "\"" << label << "\"";
            }
            s << "]";
        }
        s << "\n\t\t\t}\n";
    }

    void Router::set_latitude_longitude(const std::string& latitude, const std::string& longitude) {
        _coordinate.emplace(std::stod(latitude), std::stod(longitude));
    }
}
