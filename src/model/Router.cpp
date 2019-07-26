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
#include <rapidxml.hpp>
#include <bits/basic_string.h>
#include <sstream>
namespace mpls2pda {
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

void Router::parse_adjacency(std::istream& data, std::vector<std::unique_ptr<Router>>&routers, ptrie::map<Router*>& mapping, std::vector<const Interface*>& all_interfaces, std::ostream& warnings)
{
    rapidxml::xml_document<> doc;
    std::vector<char> buffer((std::istreambuf_iterator<char>(data)), std::istreambuf_iterator<char>());
    buffer.push_back('\0');
    doc.parse<0>(&buffer[0]);
    if (!doc.first_node()) {
        std::stringstream e;
        e << "Ill-formed XML-document for " << name() << std::endl;
        throw base_error(e.str());
    }
    auto info = doc.first_node()->first_node("isis-adjacency-information");
    if (!info) {
        std::stringstream e;
        e << "Missing an \"isis-adjacency-information\"-tag for " << name() << std::endl;
        throw base_error(e.str());
    }
    auto n = info->first_node("isis-adjacency");
    _has_config = true;
    if (n) {
        do {
            // we look up in order of ipv6, ipv4, name
            Router* next = nullptr;
            auto state = n->first_node("adjacency-state");
            if (state && strcmp(state->value(), "Up") != 0)
                continue;
            auto addv6 = n->first_node("ipv6-address");
            if (addv6) {
                auto res = mapping.exists((unsigned char*) addv6->value(), strlen(addv6->value()));
                if (!res.first) {
                    warnings << "warning: Could not find ipv6 " << addv6->value() << " in adjacency of " << name() << std::endl;
                }
                else next = mapping.get_data(res.second);

            }
            auto addv4 = n->first_node("ip-address");
            if (addv4) {
                auto res = mapping.exists((unsigned char*) addv4->value(), strlen(addv4->value()));
                if (!res.first) {
                    warnings << "warning: Could not find ipv4 " << addv4->value() << " in adjacency of " << name() << std::endl;
                }
                else if (next) {
                    auto o = mapping.get_data(res.second);
                    if (next != o) {
                        std::stringstream e;
                        e << "Mismatch between ipv4 and ipv6 in adjacency of " << name() << ", expected next router " << next->name() << " got " << o->name() << std::endl;
                        throw base_error(e.str());
                    }
                }
                else next = mapping.get_data(res.second);
            }
            auto name_node = n->first_node("system-name");
            if (name_node) {
                auto res = mapping.exists((unsigned char*) name_node->value(), strlen(name_node->value()));
                if (!res.first) {
                    warnings << "warning: Could not find name " << name_node->value() << " in adjacency of " << name() << std::endl;
                }
                else if (next) {
                    auto o = mapping.get_data(res.second);
                    if (next != o) {
                        std::stringstream e;
                        e << "Mismatch between ipv4 or ipv6 and name in adjacency of " << name() << ", expected next router " << next->name() << " got " << o->name() << std::endl;
                        throw base_error(e.str());
                    }
                }
                else next = mapping.get_data(res.second);
            }

            if (next == nullptr) {
                warnings << "warning: no matches router for adjecency-list of " << name() << " creating external node" << std::endl;
                auto nnid = routers.size();
                routers.emplace_back(std::make_unique<Router>(nnid));
                next = routers.back().get();
                if (addv4) {
                    auto res = mapping.insert((unsigned char*) addv4->value(), strlen(addv4->value()));
                    mapping.get_data(res.second) = next;
                    std::string nn = addv4->value();
                    next->add_name(nn);
                    warnings << "\t" << nn << std::endl;
                }
                if (addv6) {
                    auto res = mapping.insert((unsigned char*) addv6->value(), strlen(addv6->value()));
                    mapping.get_data(res.second) = next;
                    std::string nn = addv6->value();
                    next->add_name(nn);
                    warnings << "\t" << nn << std::endl;
                }
                if (name_node) {

                    auto res = mapping.insert((unsigned char*) name_node->value(), strlen(name_node->value()));
                    mapping.get_data(res.second) = next;
                    std::string nn = name_node->value();
                    next->add_name(nn);
                    warnings << "\t" << nn << std::endl;
                }
                next->_inferred = true;

            }

            auto iname = n->first_node("interface-name");
            if (!iname) {
                std::stringstream e;
                e << "name not given to interface on adjacency of " << name() << std::endl;
                throw base_error(e.str());
            }
            std::string iface = iname->value();
            const char* str = nullptr;
            if(auto ip = n->first_node("ip-address"))
                str = ip->value();
            auto res = get_interface(all_interfaces, iface, next, parse_ip4(str));
            if (res == nullptr) {
                std::stringstream e;
                e << "Could not find interface " << iface << std::endl;
                throw base_error(e.str());
            }
        }
        while ((n = n->next_sibling("isis-adjacency")));
    }
    else {
        std::stringstream e;
        e << "No isis-adjacency tags in " << name() << std::endl;
        throw base_error(e.str());
    }
}

Interface* Router::get_interface(std::vector<const Interface*>& all_interfaces, std::string iface, Router* expected, uint32_t ip)
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
    if (expected == this)
        ip = 0;
    auto gid = all_interfaces.size();
    _interfaces.emplace_back(std::make_unique<Interface>(iid, gid, expected, ip, this));
    all_interfaces.emplace_back(_interfaces.back().get());
    _interface_map.get_data(res.second) = _interfaces.back().get();
    return _interfaces.back().get();
}

void Router::parse_routing(std::istream& data, std::istream& indirect, std::vector<const Interface*>& all_interfaces, std::ostream& warnings, bool pfe_as_drop)
{
    rapidxml::xml_document<> doc;
    std::vector<char> buffer((std::istreambuf_iterator<char>(data)), std::istreambuf_iterator<char>());
    buffer.push_back('\0');
    doc.parse<0>(&buffer[0]);
    if (!doc.first_node()) {
        std::stringstream e;
        e << "Ill-formed XML-document for " << name() << std::endl;
        throw base_error(e.str());
    }
    auto info = doc.first_node()->first_node("forwarding-table-information");
    if (!info) {
        std::stringstream e;
        e << "Missing an \"forwarding-table-information\"-tag for " << name() << std::endl;
        throw base_error(e.str());
    }
    auto n = info->first_node("route-table");
    if (n == nullptr) {
        std::stringstream e;
        e << "Found no \"route-table\"-entries" << std::endl;
        throw base_error(e.str());
    }
    else {
        ptrie::map<std::pair < std::string, std::string>> indir;
        if (indirect.good() && indirect.peek() != EOF) {
            std::string line;
            while (std::getline(indirect, line)) {
                size_t i = 0;
                for (; i < line.size(); ++i)
                    if (line[i] != ' ') break;
                size_t j = i;
                for (; j < line.size(); ++j) {
                    if (!std::isdigit(line[j]))
                        break;
                }
                // TODO: a lot of pasta here too.
                if (j <= i) continue;
                auto rn = line.substr(i, (j - i));
                i = j;
                // find type
                for (; i < line.size(); ++i)
                    if (line[i] != ' ') break;
                // skip type
                for (; i < line.size(); ++i)
                    if (line[i] == ' ') break;
                // find eth
                for (; i < line.size(); ++i)
                    if (line[i] != ' ') break;
                j = i;
                for (; i < line.size(); ++i)
                    if (line[i] == ' ') break;
                if (i <= j) {
                    warnings << "warning; no iface for " << rn << std::endl;
                    continue;
                }
                auto iface = line.substr(j, (i - j));
                for (; i < line.size(); ++i)
                    if (line[i] != ' ') break;
                for (; i < line.size(); ++i)
                    if (line[i] == ' ') break;
                if (i == line.size()) {
                    // next line
                    if (!std::getline(indirect, line)) {
                        std::stringstream e;
                        e << "unexpected end of file" << std::endl;
                        throw base_error(e.str());
                    }
                    i = 0;
                }
                for (; i < line.size(); ++i)
                    if (line[i] != ' ') break;

                j = i;
                for (; i < line.size(); ++i)
                    if (line[i] == ' ') break;
                auto conv = line.substr(j, (i - j));
                if (conv.empty()) {
                    warnings << "warning: skip rule without protocol" << std::endl;
                    warnings << "\t" << line << std::endl;
                }

                auto res = indir.insert((const unsigned char*) rn.c_str(), rn.size());
                auto& data = indir.get_data(res.second);
                if (!res.first) {
                    if (data.first != iface) {
                        warnings << "warning: duplicate rule " << rn << " expected \"" << data.first << ":" << data.second << "\" got \"" << iface << "\"" << std::endl;
                    }
                }
                else {
                    data.first.swap(iface);
                }

                if (!res.first) {
                    if (data.second != conv) {
                        warnings << "warning: duplicate rule expected \"" << data.first << ":" << data.second << "\" got \"" << conv << "\"" << std::endl;
                    }
                }
                else {
                    data.second.swap(conv);
                }
            }
        }
        while (n) {
            try {
                _tables.emplace_back(RoutingTable::parse(n, indir, this, all_interfaces, warnings, pfe_as_drop));
            }
            catch (base_error& ex) {
                std::stringstream e;
                e << ex._message << " :: in router " << name() << std::endl;
                throw base_error(e.str());
            }
            for (size_t i = 0; i < _tables.size() - 1; ++i) {
                if (_tables.back().overlaps(_tables[i], *this, warnings)) {
                    warnings << "warning: nondeterministic routing discovered for " << name() << " in table " << n->first_node("table-name")->value() << std::endl;
                }
            }
            n = n->next_sibling("route-table");
        }
    }
}

Interface::Interface(size_t id, size_t global_id, Router* target, uint32_t ip, Router* parent) : _id(id), _global_id(global_id), _target(target), _ip(ip), _parent(parent)
{
}

void Interface::make_pairing(Router* parent, std::vector<const Interface*>& all_interfaces)
{
    if (_matching != nullptr)
        return;
    if (_target == nullptr)
        return;
    if (_target == parent)
        return;
    _matching = nullptr;
    for (auto& i : _target->_interfaces) {
        auto diff = std::max(i->_ip, _ip) - std::min(i->_ip, _ip);
        if (diff == 1 && i->_target == parent) {
            if (_matching != nullptr) {
                std::stringstream e;
                auto n = parent->interface_name(_id);
                auto n2 = _target->interface_name(_matching->_id);
                auto n3 = _target->interface_name(i->_id);
                e << "Non-unique paring of links between " << parent->name() << " and " << _target->name() << "\n";
                e << n.get() << "(";
                write_ip4(e, _ip);
                e << ") could be matched with both :\n";
                e << n2.get() << "(";
                write_ip4(e, _matching->_ip);
                e << ") and\n";
                e << n3.get() << "(";
                write_ip4(e, i->_ip);
                e << ")" << std::endl;
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
        auto n = parent->interface_name(_id);
        e << _parent->name() << "." << n.get();
        auto iface = _target->get_interface(all_interfaces, e.str(), parent, 0);
        _matching = iface;
        iface->_matching = this;
    }
}

void Router::pair_interfaces(std::vector<const Interface*>& interfaces)
{
    for (auto& i : _interfaces)
        i->make_pairing(this, interfaces);
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
    if (!_inferred) {
        if (_interfaces.size() == 0)
            out << "\"" << name() << "\" [shape=triangle];\n";
        else if (_has_config)
            out << "\"" << name() << "\" [shape=circle];\n";
        else
            out << "\"" << name() << "\" [shape=square];\n";
    }
    else {
        out << "\"" << name() << "\" [shape=square,color=red];\n";
    }

    out << "\n";

}

std::unique_ptr<char[] > Router::interface_name(size_t i)
{
    auto n = std::make_unique<char[]>(_inamelength + 1);
    auto res = _interface_map.unpack(i, (unsigned char*) n.get());
    n[res] = 0;
    return n;
}

void Router::print_json(std::ostream& s)
{
    s << "{";
    s << "\n\t\"alias\":[";
    for (size_t n = 0; n < _names.size(); ++n) {
        if (n != 0) s << ",";
        s << "\"" << _names[n] << "\"";
    }
    s << "],\n\t";
    if (!_interfaces.empty()) {
        s << "\"interfaces\":[\n\t\t";
        auto name = std::make_unique<char[]>(_inamelength + 1);
        for (size_t i = 0; i < _interfaces.size(); ++i) {
            auto id = _interfaces[i]->id();
            _interface_map.unpack(id, (unsigned char*) name.get());
            if (i != 0)
                s << ",\n\t\t";
            s << "\"" << id << "\":";
            _interfaces[i]->print_json(s, name.get());
        }
        s << "},\n\t";
    }
    if (_tables.empty()) {
        s << "\"sink\":true";
    }
    else {
        s << "\"tables\":[\n";
        for (size_t t = 0; t < _tables.size(); ++t) {
            if (t != 0)
                s << ",\n";
            _tables[t].print_json(s);
        }
        s << "]";
    }
    s << "\n}";
}

void Interface::print_json(std::ostream& s, const char* name) const
{
    s << "{\"name\":\"" << name << "\",\"virtual\":" << (_ip == 0 ? "true" : "false");
    if (_ip != 0) {
        s << ",";
        if (_target) {
            s << "\"target\":\"" << _target->name() << "\",";
            if (_matching)
                s << "\"pairing\":" << _matching->_id << ", ";
            s << "\"ip\":";
            write_ip4(s, _ip);
        }
        else
            s << "\"sink\":true";
    }
    s << "}";
}
}