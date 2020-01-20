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
 * File:   PRexBuilder.cpp
 * Author: Peter G. Jensen <root@petergjoel.dk>
 * 
 * Created on August 16, 2019, 2:44 PM
 */

#include "PRexBuilder.h"

#include <fstream>
#include <sstream>

/**
 * Note: This is a horrible, convoluted and bloated format.
 * TODO: Design a new, lean one.
 */
namespace mpls2pda {
    void PRexBuilder::open_xml(const std::string& fn, rapidxml::xml_document<>& doc, std::vector<char>& buffer) {
        std::ifstream stream(fn);
        std::stringstream es;
        if (!stream.is_open()) {
            es << "error: Could not file : " << fn << std::endl;
            throw base_error(es.str());
        }
        
        std::copy(std::istreambuf_iterator<char>(stream), std::istreambuf_iterator<char>(), std::back_inserter(buffer));
        buffer.push_back('\0');
        doc.parse<0>(&buffer[0]);
        if (!doc.first_node()) {
            es << "Ill-formed XML-document : " << fn << std::endl;
            throw base_error(es.str());
        }
    }

    Network PRexBuilder::parse(const std::string& topo_fn, const std::string& routing_fn, std::ostream& warnings)
    {
        std::vector<std::unique_ptr<Router> > routers;
        std::vector<const Interface*> interfaces;
        ptrie::map<Router*> mapping;        

        {
            rapidxml::xml_document<> topo_xml;
            std::vector<char> buffer;
            open_xml(topo_fn, topo_xml, buffer);
            build_routers(topo_xml, routers, interfaces, mapping, warnings);
        }
        
        {
            rapidxml::xml_document<> routing_xml;
            std::vector<char> buffer;
            open_xml(routing_fn, routing_xml, buffer);
            build_tables(routing_xml, routers, interfaces, mapping, warnings);
        }
        
        return Network(std::move(mapping), std::move(routers), std::move(interfaces));
    }

    
    void PRexBuilder::build_routers(rapidxml::xml_document<>& topo_xml, std::vector<std::unique_ptr<Router> >& routers, std::vector<const Interface*>& all_interfaces, ptrie::map<Router*>& mapping, std::ostream& warnings)
    {
        std::stringstream es;
        auto nw = topo_xml.first_node("network");
        if(nw == nullptr)
        {
            es << "Missing top-level <network>-tag in topology-file";
            throw base_error(es.str());
        }
        auto routers_xml = nw->first_node("routers");
        if(routers_xml == nullptr)
        {
            es << "Missing <routers>-tag in topology-file";
            throw base_error(es.str());            
        }

        auto rxml = routers_xml->first_node("router");
        while(rxml)
        {
            auto nattr = rxml->first_attribute("name");
            if(nattr == nullptr)
            {
                es << "Missing \"name\" attribute in router-node in xml";
                throw base_error(es.str());
            }
            std::string name = nattr->value();
            size_t id = routers.size();
            routers.emplace_back(std::make_unique<Router>(id));
            Router& router = *routers.back().get();
            router.add_name(name);
            auto res = mapping.insert((const unsigned char*)name.c_str(), name.length());
            if(!res.first)
            {
                es << "error: Duplicate definition of \"" << name << "\", previously found in entry " << mapping.get_data(res.second)->index() << std::endl;
                throw base_error(es.str());
            }
            mapping.get_data(res.second) = &router;
            auto interfaces = rxml->first_node("interfaces");
            if(interfaces)
            {
                auto inf = interfaces->first_node("interface");
                while(inf)
                {
                    auto nattr = inf->first_attribute("name");
                    if(nattr == nullptr)
                    {
                        es << "Interface is missing a \"name\" attribute";
                        throw base_error(es.str());
                    }
                    std::string iname = nattr->value();
                    router.get_interface(all_interfaces, iname); // will add the interface
                    inf = inf->next_sibling("interface");
                }
            }
            rxml = rxml->next_sibling("router");
        }
        
        auto links_xml = nw->first_node("links");
        if(links_xml == nullptr)
        {
            es << "Missing <links>-tag in topology-file";
            throw base_error(es.str());            
        }
                
        auto link = links_xml->first_node("link");
        while(link)
        {
            auto side = link->first_node("sides");
            if(side == nullptr)
            {
                es << "<link>-tag must contain a <sides>-tag";
                throw base_error(es.str());
            }
            auto inf1 = side->first_node("shared_interface");
            if(inf1 == nullptr)
            {
                es << "Missing <shared_interface>-tag in <link>";
                throw base_error(es.str());                
            }
            auto inf2 = inf1->next_sibling("shared_interface");
            if(inf2 == nullptr)
            {
                es << "Missing second <shared_interface>-tag in <link>";
                throw base_error(es.str());                
            }
            if(inf2->next_sibling("shared_interface"))
            {
                es << "A <link>-tag must contain exactly two <shared_interface>-tags, more detected.";
                throw base_error(es.str());
            }
            
            Interface* inter1 = nullptr;
            Interface* inter2 = nullptr;
            for(auto& inf : {inf1, inf2})
            {
                auto iattr = inf->first_attribute("interface");
                auto rattr = inf->first_attribute("router");
                if(iattr == nullptr || rattr == nullptr)
                {
                    es << "A <shared_interface>-tag must contain both \"interface\" and \"router\" attribute";
                    throw base_error(es.str());
                }
                std::string rn = rattr->value();
                std::string in = iattr->value();
                auto res = mapping.exists((unsigned char*)rn.c_str(), rn.length());
                if(!res.first)
                {
                    es << "Could not find router " << rn << " for matching of links.";
                    throw base_error(es.str());
                }
                auto& router = mapping.get_data(res.second);
                auto interface = router->find_interface(in);
                if(interface == nullptr)
                {
                    es << "Could not find interface " << in << " for matching of links in router " << rn;
                    throw base_error(es.str());                    
                }
                (inter1 == nullptr ? inter1 : inter2) = interface;
            }
            inter1->make_pairing(inter2);
            assert(inter1->match() == inter2);
            assert(inter2->match() == inter1);
            link = link->next_sibling("link");
        }
        Router* nullrouter = nullptr;
        for(auto& r : routers)
        {
            for(auto& inf : r->interfaces())
            {
                if(inf->match() == nullptr)
                {
                    if(nullrouter == nullptr)
                    {
                        size_t id = routers.size();
                        routers.emplace_back(std::make_unique<Router>(id));
                        Router& router = *routers.back().get();
                        router.add_name("NULL");
                        auto res = mapping.insert((const unsigned char*)"NULL", 4);
                        if(!res.first)
                        {
                            es << "error: Duplicate definition of \"NULL\", previously found in entry " << mapping.get_data(res.second)->index() << std::endl;
                            throw base_error(es.str());
                        }
                        mapping.get_data(res.second) = &router;
                        nullrouter = routers.back().get();
                    }
                    std::stringstream ss;
                    ss << "i" << inf->global_id();
                    auto interface = nullrouter->get_interface(all_interfaces, ss.str(), r.get()); // will add the interface
                    if(interface == nullptr)
                    {
                        es << "Could not find interface " << ss.str() << " for matching of links in router " << nullrouter->name();
                        throw base_error(es.str());                    
                    }
                    interface->make_pairing(inf.get());
                }
            }
        }
    }

    void PRexBuilder::build_tables(rapidxml::xml_document<>& topo_xml, std::vector<std::unique_ptr<Router> >& routers, std::vector<const Interface*>& interfaces, ptrie::map<Router*>& mapping, std::ostream& warnings)
    {
        std::stringstream es;
        auto nw = topo_xml.first_node("routes");
        if(nw == nullptr)
        {
            es << "Missing top-level <routes>-tag in routing-file";
            throw base_error(es.str());
        }
        auto routers_xml = nw->first_node("routings");
        if(routers_xml == nullptr)
        {
            es << "Missing <routings>-tag in topology-file";
            throw base_error(es.str());            
        }
        
        auto router_table = routers_xml->first_node("routing");
        while(router_table)
        {
            auto for_attr = router_table->first_attribute("for");
            if(for_attr == nullptr)
            {
                es << "Missing \"for\"-attribute in <routing>-tag in routing-file";
                throw base_error(es.str());
            }
            std::string rn = for_attr->value();
            auto res = mapping.exists((unsigned char*)rn.c_str(), rn.length());
            if(!res.first)
            {
                es << "Could not find router " << rn << " for building routing-tables.";
                throw base_error(es.str());
            }
            auto router = mapping.get_data(res.second);
            
            auto dests = router_table->first_node("destinations");
            if(dests != nullptr)
            {
                auto dest = dests->first_node("destination");
                while(dest)
                {
                    auto ifattr = dest->first_attribute("from");
                    auto lblattr = dest->first_attribute("label");
                    auto ipattr = dest->first_attribute("ip");
                    Interface* inf = nullptr;
                    if(lblattr != nullptr && ipattr != nullptr)
                    {
                        es << "\"label\" and \"ip\" cannot be applied at the same time";
                        throw base_error(es.str());
                    }
                    if(ifattr != nullptr)
                    {
                        inf = router->find_interface(ifattr->value());
                        if(inf == nullptr)
                        {
                            es << "Could not find match for \"interface\" on <destination> : " << ifattr->value();
                            throw base_error(es.str());
                        }
                    }
                    RoutingTable table;
                    auto& entry = table.push_entry();
                    entry._ingoing = inf;
                    if(lblattr != nullptr)
                    {
                        auto val = lblattr->value();
                        if(strlen(val) == 0)
                        {
                            entry._top_label.set_value(Query::ANYIP, 0, 0);
                        }
                        else
                        {
                            auto sticky = dest->first_attribute("sticky");
                            Query::type_t type = Query::MPLS;
                            if(sticky != nullptr && strcmp(sticky->value(), "1") == 0)
                                type = Query::STICKY_MPLS;
                            entry._top_label.set_value(type, atoi(lblattr->value()), 0);
                            
                        }
                    }
                    else if(ipattr != nullptr)
                    {
                        std::string add = ipattr->value();
                        bool ok = false;
                        for(auto c : add)
                        {
                            // this is too simple, FIX!
                            if(c == '.')
                            {
                                entry._top_label.set_value(Query::IP4, parse_ip4(add.c_str()), 0);
                                ok = true;
                                break;
                            }
                            else if(c == ':')
                            {
                                entry._top_label.set_value(Query::IP6, parse_ip6(add.c_str()), 0);
                                ok = true;
                                break;
                            }
                        }
                        if(!ok)
                        {
                            es << add << " is not a valid ip";
                            throw base_error(es.str());
                        }
                        ok = false;
                        size_t i = 0; 
                        for(;i < add.size(); ++i) if(add[i] == '/') break;
                        ++i;
                        if(i < add.size())
                        {
                            entry._top_label.set_mask(atoi(&add[i]));
                        }
                        
                    }
                    assert(!ipattr);
                    auto tegrps = dest->first_node("te-groups");
                    if(tegrps) // could be empty? // Could be more?
                    {
                        size_t weight = 0;
                        auto grp = tegrps->first_node("te-group");
                        while(grp)
                        {
                            bool any = false;
                            auto routes = grp->first_node("routes");
                            if(routes)
                            {
                                auto route = routes->first_node("route");
                                while(route)
                                {
                                    any = true;
                                    auto toattr = route->first_attribute("to");
                                    if(toattr == nullptr)
                                    {
                                        es << "Could not find \"to\" attribute in <route>-tag";
                                        throw base_error(es.str());
                                    }
                                    entry._rules.emplace_back();
                                    entry._rules.back()._via = router->find_interface(toattr->value());
                                    entry._rules.back()._type = RoutingTable::MPLS;
                                    entry._rules.back()._weight = weight;

                                    if(entry._rules.back()._via == nullptr)
                                    {
                                        es << "Could not match find \"to\" with interface on " << router->name() << "attribute in <route>-tag : " << toattr->value();
                                        throw base_error(es.str());
                                    }
                                    auto actions = route->first_node("actions");
                                    if(actions != nullptr || actions->first_node("action") == nullptr)
                                    {
                                        auto action = actions->first_node("action");
                                        while(action)
                                        {
                                            auto tattr = action->first_attribute("type");
                                            auto aattr = action->first_attribute("arg");
                                            if(tattr == nullptr)
                                            {
                                                es << "Missing \"type\" on action";
                                                throw base_error(es.str());
                                            }
                                            std::string type = tattr->value();
                                            bool ok = false;
                                            entry._rules.back()._ops.emplace_back();
                                            auto& op = entry._rules.back()._ops.back();
                                            if(type == "pop")
                                            {
                                                if(aattr != nullptr)
                                                {
                                                    es << type << " ignoes \"arg\"";
                                                    throw base_error(es.str());
                                                }
                                                ok = true;
                                                op._op = RoutingTable::POP;
                                            }
                                            else
                                            {
                                                if(aattr == nullptr)
                                                {
                                                    es << type << " needs an \"arg\"";
                                                    throw base_error(es.str());
                                                }
                                                op._op_label.set_value(Query::MPLS, std::atoi(aattr->value()), 0);
                                                if(type == "push")
                                                {
                                                    op._op = RoutingTable::PUSH;
                                                    ok = true;
                                                }
                                                else if(type == "swap")
                                                {
                                                    op._op = RoutingTable::SWAP;
                                                    ok = true;
                                                }
                                                else
                                                {
                                                    assert(false);
                                                }
                                            }
                                            if(!ok)
                                            {
                                                es << "Unknown operator : \"" << type << "\"";
                                                throw base_error(es.str());
                                            }
                                            action = action->next_sibling("action");
                                        }
                                    }
                                    else
                                    {
                                        // no-op
                                    }
                                    route = route->next_sibling("route");
                                }
                            }
                            
                            grp = grp->next_sibling("te-group");
                            if(any) ++weight;
                        }
                    }
                    table.sort();
                    std::stringstream e;
                    if(!table.check_nondet(e))
                    {
                        throw base_error(e.str());
                    }
                    inf->table().merge(table, *inf, warnings);
                    dest = dest->next_sibling("destination");
                }
            }
            router_table = router_table->next_sibling("routing");
        }
    }
}

