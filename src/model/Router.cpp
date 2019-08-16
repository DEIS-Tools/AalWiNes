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
 * File:   Router.cpp
 * Author: Peter G. Jensen <root@petergjoel.dk>
 * 
 * Created on June 24, 2019, 11:22 AM
 */

#include "Router.h"
#include "utils/errors.h"
#include "utils/parsing.h"

#include <vector>
#include <streambuf>
#include <sstream>
namespace mpls2pda
{

    Router::Router(size_t id) : _index(id)
    {
    }

    void Router::add_name(const std::string& name)
    {
        _names.emplace_back(name);
    }

    const std::string& Router::name() const
    {
        assert(_names.size() > 0);
        return _names.back();
    }

    Interface* Router::get_interface(std::vector<const Interface*>& all_interfaces, std::string iface, Router* expected)
    {
        for (size_t i = 0; i < iface.size(); ++i) {
            if (iface[i] == ' ') {
                iface = iface.substr(0, i);
                break;
            }
        }
        size_t l = strlen(iface.c_str());
        _inamelength = std::max(_inamelength, l);
        auto res = _interface_map.insert((unsigned char*) iface.c_str(), iface.length());
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
        all_interfaces.emplace_back(_interfaces.back().get());
        _interface_map.get_data(res.second) = _interfaces.back().get();
        return _interfaces.back().get();
    }

    Interface::Interface(size_t id, size_t global_id, Router* target, Router* parent) : _id(id), _global_id(global_id), _target(target), _parent(parent)
    {
    }

    void Interface::make_pairing(std::vector<const Interface*>& all_interfaces, std::function<bool(const Interface*, const Interface*)> matcher)
    {
        if (_matching != nullptr)
            return;
        if (_target == nullptr)
            return;
        if (_target == _parent)
            return;
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
                    e << n.get() << " could be matched with both :\n";
                    e << n2.get() << " and\n";
                    e << n3.get() << std::endl;
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
            e << _parent->name() << "." << n.get();
            auto iface = _target->get_interface(all_interfaces, e.str(), _parent);
            _matching = iface;
            iface->_matching = this;
        }
    }

    void Router::pair_interfaces(std::vector<const Interface*>& interfaces, std::function<bool(const Interface*, const Interface*)> matcher)
    {
        for (auto& i : _interfaces)
            i->make_pairing(interfaces, matcher);
    }

    void Router::print_dot(std::ostream& out)
    {
        if (_interfaces.empty())
            return;
        auto n = std::make_unique<char[]>(_inamelength + 1);
        for (auto& i : _interfaces) {
            auto res = _interface_map.unpack(i->id(), (unsigned char*) n.get());
            n[res] = 0;
            auto tgtstring = i->target() != nullptr ? i->target()->name() : "SINK";
            out << "\"" << name() << "\" -> \"" << tgtstring
                    << "\" [ label=\"" << n.get() << "\" ];\n";
        }
        if (_interfaces.size() == 0)
            out << "\"" << name() << "\" [shape=triangle];\n";
        else
            out << "\"" << name() << "\" [shape=circle];\n";

        out << "\n";

    }

    std::unique_ptr<char[] > Router::interface_name(size_t i)
    {
        auto n = std::make_unique<char[]>(_inamelength + 1);
        auto res = _interface_map.unpack(i, (unsigned char*) n.get());
        n[res] = 0;
        return n;
    }

}