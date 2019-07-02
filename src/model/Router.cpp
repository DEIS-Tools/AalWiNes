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

#include <vector>
#include <streambuf>
#include <rapidxml.hpp>

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

bool Router::parse_adjacency(std::istream& data, std::vector<std::unique_ptr<Router>>& routers, ptrie::map<Router*>& mapping)
{
    rapidxml::xml_document<> doc;
    std::vector<char> buffer((std::istreambuf_iterator<char>(data)), std::istreambuf_iterator<char>());
    buffer.push_back('\0');
    doc.parse<0>(&buffer[0]);
    if(!doc.first_node())
    {
        std::cerr << "Ill-formed XML-document for " << name() << std::endl;
        return false;
    }
    auto info = doc.first_node()->first_node("isis-adjacency-information");
    if(!info)
    {
        std::cerr << "Missing an \"isis-adjacency-information\"-tag for " << name() << std::endl;
        return false;
    }
    auto n = info->first_node("isis-adjacency");
    _has_config = true;
    if(n) {
        do {
            // we look up in order of ipv6, ipv4, name
            Router* next = nullptr;
            auto state = n->first_node("adjacency-state");
            if(state && strcmp(state->value(), "Up") != 0)
                continue;
            auto addv6 = n->first_node("ipv6-address");
            if(addv6)
            {
                auto res = mapping.exists((unsigned char*)addv6->value(), strlen(addv6->value()));
                if(!res.first)
                {
                    std::cerr << "warning: Could not find ipv6 " << addv6->value() << " in adjacency of " << name() << std::endl;
                }
                else next = mapping.get_data(res.second);
                    
            }
            auto addv4 = n->first_node("ip-address");
            if(addv4)
            {
                auto res = mapping.exists((unsigned char*)addv4->value(), strlen(addv4->value()));
                if(!res.first)
                {
                    std::cerr << "warning: Could not find ipv4 " << addv4->value() << " in adjacency of " << name() << std::endl;
                }
                else if(next)
                {
                    auto o = mapping.get_data(res.second);
                    if(next != o)
                    {
                        std::cerr << "error: Mismatch between ipv4 and ipv6 in adjacency of " << name() << ", expected next router " << next->name() << " got " << o->name() << std::endl;
                        return false;
                    }
                }
                else next = mapping.get_data(res.second);
            }
            auto name_node = n->first_node("system-name");
            if(name_node)
            {
                auto res = mapping.exists((unsigned char*)name_node->value(), strlen(name_node->value()));
                if(!res.first)
                {
                    std::cerr << "warning: Could not find name " << name_node->value() << " in adjacency of " << name() << std::endl;
                }
                else if(next)
                {
                    auto o = mapping.get_data(res.second);
                    if(next != o)
                    {
                        std::cerr << "error: Mismatch between ipv4 or ipv6 and name in adjacency of " << name() << ", expected next router " << next->name() << " got " << o->name() << std::endl;
                        return false;
                    }
                }
                else next = mapping.get_data(res.second);
            }
            
            if(next == nullptr)
            {
                std::cerr << "warning: no matches router for adjecency-list of " << name() << " creating external node" << std::endl;
                auto nnid = routers.size();
                routers.emplace_back(std::make_unique<Router>(nnid));
                next = routers.back().get();
                if(addv4)
                {
                    auto res = mapping.insert((unsigned char*)addv4->value(), strlen(addv4->value()));
                    mapping.get_data(res.second) = next;
                    std::string nn = addv4->value();
                    next->add_name(nn);
                    std::cerr << "\t" << nn << std::endl;
                }
                if(addv6)
                {
                    auto res = mapping.insert((unsigned char*)addv6->value(), strlen(addv6->value()));
                    mapping.get_data(res.second) = next;
                    std::string nn = addv6->value();
                    next->add_name(nn);
                    std::cerr << "\t" << nn << std::endl;
                }
                if(name_node)
                {

                    auto res = mapping.insert((unsigned char*)name_node->value(), strlen(name_node->value()));
                    mapping.get_data(res.second) = next;
                    std::string nn = name_node->value();
                    next->add_name(nn);
                    std::cerr << "\t" << nn << std::endl;
                }
                next->_inferred = true;

            }

            auto iname = n->first_node("interface-name");
            if(!iname)
            {
                std::cerr << "error: name not given to interface on adjacency of " << name() << std::endl;
                return 0;
            }
            size_t l = strlen(iname->value());
            _inamelength = std::max(_inamelength, l);
            auto res = _interface_map.insert((unsigned char*)iname->value(), l);
            if(!res.first && _interface_map.get_data(res.second)->target() != next)
            {
                std::cerr << "error: duplicate interface declaration, got " << _interface_map.get_data(res.second)->target()->name() << " new target " << next->name() << " on adjacency of " << name() << std::endl;
                return false;
            }
            auto iid = _interfaces.size();
            _interfaces.emplace_back(std::make_unique<Interface>(iid, next));
            _interface_map.get_data(res.second) = _interfaces.back().get();
        } while((n = n->next_sibling("isis-adjacency")));
    }
    else
    {
        std::cerr << "error: No isis-adjacency tags in " << name() << std::endl;
        return false;
    }
    return true;
}

void Router::print_dot(std::ostream& out)
{
    if(_interfaces.empty())
        return;
    auto n = std::make_unique<char[]>(_inamelength+1);
    for(auto& i : _interfaces)
    {
        auto res = _interface_map.unpack(i->id(), (unsigned char*)n.get());
        n[res] = 0;
        out << "\"" << name() << "\" -> \"" << i->target()->name() 
            << "\" [ label=\"" << n.get() << "\" ];\n";
    }
    if(!_inferred)
    {
        if(_interfaces.size() == 0)
            out << "\"" << name() << "\" [shape=triangle];\n";
        else if(_has_config)
            out << "\"" << name() << "\" [shape=circle];\n";
        else
            out << "\"" << name() << "\" [shape=square];\n";
    }
    else
    {
        out << "\"" << name() << "\" [shape=square,color=red];\n";
    }
        
    out << "\n";

}




