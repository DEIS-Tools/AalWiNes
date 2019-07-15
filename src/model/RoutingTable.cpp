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
 * File:   RoutingTable.cpp
 * Author: Peter G. Jensen <root@petergjoel.dk>
 * 
 * Created on July 2, 2019, 3:29 PM
 */

#include "RoutingTable.h"
#include "Router.h"

#include <algorithm>

RoutingTable::RoutingTable()
{
}

RoutingTable RoutingTable::parse(rapidxml::xml_node<char>* node, ptrie::map<std::pair<std::string,std::string>>& indirect, Router* parent)
{
    RoutingTable nr;
    if(node == nullptr)
        return nr;
    
    
    if(auto nn = node->first_node("table-name"))
        nr._name = nn->value();
    
    if(auto tn = node->first_node("address-family"))
    {
        if(strcmp(tn->value(), "MPLS") != 0)
        {
            std::cerr << "error: Not MPLS-type address-family routing-table (\"" << nr._name << "\")" << std::endl;
            return nr;
        }
    }
    
    auto rule = node->first_node("rt-entry");
    if(rule == nullptr)
    {
        std::cerr << "error: no entries in routing-table \"" << nr._name << "\"" << std::endl;
        return nr;
    }
    
    std::vector<std::pair<std::string,std::pair<size_t,size_t>>> lsi;
    
    while(rule)
    {
        std::string tl = rule->first_node("rt-destination")->value();
        nr._entries.emplace_back();
        auto pos = tl.find("(S=0)");
        auto& entry = nr._entries.back();

        if(pos != std::string::npos)
        {
            if(pos != tl.size() - 5)
            {
                std::cerr << "error: expect only (S=0) notation as postfix of <rt-destination> in table " << nr._name << " of router " << parent->name() << std::endl;
                nr._entries.clear();
                return nr;
            }
            entry._decreasing = true;
            tl = tl.substr(0, pos);
        }
        if(std::all_of(std::begin(tl), std::end(tl), [](auto& c) { return std::isdigit(c);}))
        {
            entry._top_label = atoll(tl.c_str()) + 1;
        }
        else if(tl == "default")
        {
            entry._top_label = 0;
        }
        else
        {
            entry._top_label = (-1*(int64_t)parent->get_interface(tl)->id())-1;
        }

        auto nh = rule->first_node("nh");
        if(nh == nullptr)
        {
            std::cerr << "error: no \"nh\" entries in routing-table \"" << nr._name << "\" for \"" << entry._top_label << "\"" << std::endl;
            return nr;
        }
        int cast = 0;
        
        do {
            entry._rules.emplace_back();
            auto& r = entry._rules.back();
            auto nhweight = nh->first_node("nh-weight");
            if(nhweight)
            {
                r._weight = atoll(nhweight->value());
            }
            auto ops = nh->first_node("nh-type");
            bool skipvia = true;
            rapidxml::xml_node<>* nhid = nullptr;
            if(ops)
            {
                auto val = ops->value();
                if(strcmp(val, "unilist") == 0)
                {
                    if(cast != 0)
                    {
                        std::cerr << "error: already in cast" << std::endl;
                        nr._entries.clear();
                        return nr;
                    }
                    cast = 1;
                    entry._rules.pop_back();
                    continue;
                }
                else if(strcmp(val, "discard") == 0)
                {
                    r._type = DISCARD; // Drop
                }
                else if(strcmp(val, "receive") == 0) // check.
                {
                    r._type = RECIEVE; // Drops out of MPLS?
                }
                else if(strcmp(val, "table lookup") == 0)
                {
                    r._type = ROUTE; // drops out of MPLS?
                }
                else if(strcmp(val, "indirect") == 0)
                {
                    skipvia = false;
                    // lookup in indirect
                    nhid = nh->first_node("nh-index");
                    if(!nhid)
                    {
                        std::cerr << "error: expected nh-index of indirect";
                        nr._entries.clear();
                        return nr;
                    }
                }
                else if(strcmp(val, "unicast") == 0)
                {
                    skipvia = false;
                    // no ops, though.
                }
                else
                {
                    std::string ostr = ops->value();
                    auto pos = ostr.find("(top)");
                    if(pos != std::string::npos)
                    {
                        if(pos != ostr.size() - 5)
                        {
                            std::cerr << "error: expected \"(top)\" predicate at the end of <nh-type> only. Error in routing-table for router " << parent->name() << std::endl;
                            nr._entries.clear();
                            return nr;
                        }
                        ostr = ostr.substr(0,pos);
                    }
                    // parse ops
                    bool parse_label = false;
                    r._ops.emplace_back();
                    for(size_t i = 0; i < ostr.size(); ++i)
                    {
                        if(ostr[i] == ' ') continue;
                        if(ostr[i] == ',') continue;
                        if(!parse_label)
                        {
                            if(ostr[i] == 'S')
                            {
                                r._ops.back()._op = SWAP;
                                i += 4;
                                parse_label = true;
                            }
                            else if(ostr[i] == 'P')
                            {
                                if(ostr[i+1] == 'u')
                                {
                                    r._ops.back()._op = PUSH;
                                    parse_label = true;
                                    i += 4;
                                }
                                else if(ostr[i+1] == 'o')
                                {
                                    r._ops.back()._op = POP;
                                    i += 2;
                                    continue;                                   
                                }
                            }

                            if(parse_label)
                            {
                                while(i < ostr.size() && ostr[i] == ' ')
                                {
                                    ++i;
                                }
                                if(i != ostr.size() && std::isdigit(ostr[i]))
                                {
                                    size_t j = i;
                                    while(j < ostr.size() && std::isdigit(ostr[j]))
                                    {
                                        ++j;
                                    }
                                    auto n = ostr.substr(i, (j-i));
                                    r._ops.back()._label = std::atoi(n.c_str());
                                    i = j;
                                    parse_label = false;
                                    continue;
                                }
                            }
                            std::cerr << "error: unexpected operation type \"" << (&ostr[i]) << "\". Error in routing-table for router " << parent->name() << std::endl;
                            nr._entries.clear();
                            return nr;                                
                        }
                    }
                    skipvia = false;
                }
            }
            
            auto via = nh->first_node("via");
            if(via && strlen(via->value()) > 0)
            {
                if(skipvia)
                {
                    std::cerr << "warning: found via \"" << via->value() << "\" in \"" << nr._name << "\" for \"" << entry._top_label << "\"" << std::endl;
                    std::cerr << "\t\tbut got type expecting no via: " << ops->value() << std::endl;                    
                }
                else
                {
                    std::string iname = via->value();
                    for(size_t i = 0; i < iname.size(); ++i)
                    {
                        if(iname[i] == ' ')
                        {
                            iname = iname.substr(0,i);
                            break;
                        }
                    }
                    if(iname.find("lsi.") == 0)
                    {
                        // self-looping interface
                        r._via = parent->get_interface(iname, parent);
                    }
                    else
                    {
                        r._via = parent->get_interface(iname);
                    }
                }
            }
            else if(!skipvia && indirect.size() > 0)
            {
                if(nhid)
                {
                    auto val = nhid->value();
                    auto alt = indirect.exists((const unsigned char*)val, strlen(val));
                    if(!alt.first)
                    {
                        std::cerr << "error: Could not lookup indirect : " << val << std::endl;
                        std::cerr << "\ttype : " << ops->value() << std::endl;
                        nr._entries.clear();
                        return nr;
                    }
                    else
                    {
                        auto& d = indirect.get_data(alt.second);
                        r._via = parent->get_interface(d.first);
                    }
                }
                else
                {
                    std::cerr << "warning: found no via in \"" << nr._name << "\" for \"" << entry._top_label << "\"" << std::endl;
                    std::cerr << "\t\tbut got type: " << ops->value() << std::endl;
                    std::cerr << std::endl;
                }
            }
        } while((nh = nh->next_sibling("nh")));
        rule = rule->next_sibling("rt-entry");
    }
    std::sort(nr._entries.begin(), nr._entries.end());
    for(size_t i = 1 ; i < nr._entries.size(); ++i)
    {
        if(nr._entries[i-1] == nr._entries[i])
        {
            std::cerr << "warning: nondeterministic routing-table found, dual matches on " << nr._entries[i]._top_label << " for router " << parent->name() << std::endl;
        }
    }
    return nr;
}

bool RoutingTable::overlaps(const RoutingTable& other, Router& parent) const
{
    auto oit = other._entries.begin();
    for(auto& e : _entries)
    {

        while(oit != std::end(other._entries) && (*oit) < e)
            ++oit;
        if(oit == std::end(other._entries))
            return false;
        if((*oit) == e)
        {
            auto label = e._top_label;
            std::cerr << "\t\tOverlap on label ";
            if(label > 0)
                std::cerr << (label-1) << std::endl;
            else if(label == 0)
            {
                std::cerr << "default" << std::endl;
            }
            else
            {
                auto id = -1*(label+1);
                auto iname = parent.interface_name(id);
                std::cerr << iname.get() << " (" << id << ")" << std::endl;                
            }
            std::cerr << " for router " << parent.name() << std::endl;
            return true;
        }
    }
    return false;
}

std::ostream& RoutingTable::action_t::operator<<(std::ostream& s) const
{
    switch(_op)
    {
    case SWAP:
        s << "Swap " << _label;
        break;
    case PUSH:
        s << "Push " << _label;
        break;
    case POP:
        s << "Pop";
        break;
    }
    return s;
}
