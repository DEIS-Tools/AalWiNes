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
#include <sstream>

namespace aalwines
{

    Network::Network(routermap_t&& mapping, std::vector<std::unique_ptr<Router> >&& routers, std::vector<const Interface*>&& all_interfaces)
    : _mapping(std::move(mapping)), _routers(std::move(routers)), _all_interfaces(std::move(all_interfaces))
    {
        ptrie::set<Query::label_t> all_labels;
        _total_labels = 0;
        for (auto& r : _routers) {
            for (auto& inf : r->interfaces()) {
                for (auto& e : inf->table().entries()) {
                    if(all_labels.insert(e._top_label).first) ++_total_labels;
                }
            }
        }
    }

    Router* Network::get_router(size_t id) {
        if (id < _routers.size()) {
            return _routers[id].get();
        }
        else {
            return nullptr;
        }
    }

    const char* empty_string = "";

    std::unordered_set<Query::label_t> Network::interfaces(filter_t& filter)
    {
        std::unordered_set<Query::label_t> res;
        for (auto& r : _routers) {
            if (filter._from(r->name().c_str())) {
                for (auto& i : r->interfaces()) {
                    if (i->is_virtual()) continue;
                    // can we have empty interfaces??
                    assert(i);
                    auto fname = r->interface_name(i->id());
                    std::unique_ptr<char[] > tname = std::make_unique<char[]>(1);
                    tname.get()[0] = '\0';
                    const char* tr = empty_string;
                    if (i->target() != nullptr) {
                        if (i->match() != nullptr) {
                            tname = i->target()->interface_name(i->match()->id());
                        }
                        tr = i->target()->name().c_str();
                    }
                    if (filter._link(fname.get(), tname.get(), tr)) {
                        res.insert(Query::label_t{Query::INTERFACE, 0, i->global_id()}); // TODO: little hacksy, but we have uniform types in the parser
                    }
                }
            }
        }
        return res;
    }

    std::unordered_set<Query::label_t> Network::get_labels(uint64_t label, uint64_t mask, Query::type_t type, bool exact)
    {
        std::unordered_set<Query::label_t> res;
        for (auto& pr : all_labels()) {
            if (pr.type() != type) continue;
            auto msk = std::max<uint8_t>(mask, pr.mask());
            if(pr == Query::label_t::unused_ip4 ||
               pr == Query::label_t::unused_ip6 ||
               pr == Query::label_t::unused_mpls ||
               pr == Query::label_t::unused_sticky_mpls ||
               pr == Query::label_t::any_ip ||
               pr == Query::label_t::any_ip4 ||
               pr == Query::label_t::any_ip6 ||
               pr == Query::label_t::any_mpls ||
               pr == Query::label_t::any_sticky_mpls) continue;
            switch (type) {
            case Query::IP6:
            case Query::IP4:
            case Query::STICKY_MPLS:
            case Query::MPLS:
            {
                if ((pr.value() << msk) == (label << msk) && 
                     (pr.type() & Query::STICKY) == (type & Query::STICKY))
                {
                    res.insert(pr);
                }
            }
            default:
                break;
            }
        }
        if(res.empty())
        {
            switch (type) {
            case Query::IP4:
                res.emplace(Query::label_t::unused_ip4);
                break;
            case Query::IP6:
                res.emplace(Query::label_t::unused_ip6);
                break;
            case Query::STICKY_MPLS:
                res.emplace(Query::label_t::unused_sticky_mpls);
                break;
            case Query::MPLS:
                res.emplace(Query::label_t::unused_mpls);
                break;
            default:
                throw base_error("Unknown expansion");
            }
        }
        if(!exact)
        {
            switch (type) {
            case Query::IP4:
                res.emplace(Query::label_t::any_ip4);
                break;
            case Query::IP6:
                res.emplace(Query::label_t::any_ip6);
                break;
            case Query::STICKY_MPLS:
                res.emplace(Query::label_t::any_sticky_mpls);
                break;
            case Query::MPLS:
                res.emplace(Query::label_t::any_mpls);
                break;
            default:
                throw base_error("Unknown expansion");
            }        
        }
        return res;
    }

    std::unordered_set<Query::label_t> Network::all_labels()
    {
        if(_label_cache.size() == 0)
        {
            std::unordered_set<Query::label_t> res;
            res.reserve(_total_labels + 7);
            res.insert(Query::label_t::unused_ip4);
            res.insert(Query::label_t::unused_ip6);
            res.insert(Query::label_t::unused_mpls);
            res.insert(Query::label_t::unused_sticky_mpls);
            res.insert(Query::label_t::any_ip);
            res.insert(Query::label_t::any_ip4);
            res.insert(Query::label_t::any_ip6);
            res.insert(Query::label_t::any_mpls);
            res.insert(Query::label_t::any_sticky_mpls);
            _non_service_label = res;
            for (auto& r : _routers) {
                for (auto& inf : r->interfaces()) {
                    for (auto& e : inf->table().entries()) {
                        res.insert(e._top_label);
                        for (auto& f : e._rules) {
                            for (auto& o : f._ops) {
                                switch (o._op) {
                                case RoutingTable::SWAP:
                                case RoutingTable::PUSH:
                                    res.insert(o._op_label);
                                    _non_service_label.insert(o._op_label);
                                default:
                                    break;
                                }
                            }
                        }
                    }
                }
            }
            _label_cache = res;
        }
        return _label_cache;
    }

    void Network::move_network(Network&& nested_network){
        // Find NULL router
        auto res = _mapping.insert("NULL", 4);
        assert(!res.first);
        auto nullrouter = _mapping.get_data(res.second);

        // Move old network into new network.
        for (auto&& e : nested_network._routers) {
            if (e->is_null()) {
                continue;
            }
            // Find unique name for router
            std::string name = e->name();
            while(_mapping.exists(name.c_str(), name.length()).first){
                name += "'";
            }
            e->change_name(name);

            // Add interfaces to _all_interfaces and update their global id.
            for (auto&& inf: e->interfaces()) {
                inf->update_global_id(_all_interfaces.size());
                _all_interfaces.push_back(inf.get());
                // Transfer links from old NULL router to new NULL router.
                if (inf->target()->is_null()){
                    std::stringstream ss;
                    ss << "i" << inf->global_id();
                    auto interface = nullrouter->get_interface(_all_interfaces, ss.str(), e.get()); // will add the interface
                    interface->make_pairing(inf.get());
                }
            }

            // Move router to new network
            e->update_index(_routers.size());
            _routers.emplace_back(std::move(e));
            _mapping.get_data(_mapping.insert(name.c_str(), name.length()).second) = _routers.back().get();
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
        auto virtual_guard = nested_outgoing->source()->get_interface(_all_interfaces, "__virtual_guard__"); // Assumes these names are unique for this router.
        auto nested_end_link = nested_outgoing->source()->get_interface(_all_interfaces, "__end_link__");
        nested_outgoing->make_pairing(virtual_guard);
        link_end->make_pairing(nested_end_link);

        move_network(std::move(nested_network));

        // Add push and pop rules.
        for (auto&& interface : link->source()->interfaces()) {
            interface->table().add_to_outgoing(link,
                    {RoutingTable::op_t::PUSH, pre_label});
        }
        virtual_guard->table().add_rule(post_label, {RoutingTable::op_t::POP, RoutingTable::label_t{}}, nested_end_link);
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

    void Network::print_dot(std::ostream& s)
    {
        s << "digraph network {\n";
        for (auto& r : _routers) {
            r->print_dot(s);
        }
        s << "}" << std::endl;
    }
    void Network::print_simple(std::ostream& s)
    {
        for(auto& r : _routers)
        {
            s << "router: \"" << r->name() << "\":\n";
            r->print_simple(s);
        }
    }
    void Network::print_json(std::ostream& s)
    {
        s << "\t\"routers\": {\n";
        bool first = true;
        for(auto& r : _routers)
        {
            if (r->is_null()) {
                continue;
            }
            if (first) {
                first = false;
            } else {
                s << ",\n";
            }            
            s << "\t\t\"" << r->name() << "\": {\n";
            r->print_json(s);
            s << "\t\t}";
        }
        s << "\n\t}";
    }

    bool Network::is_service_label(const Query::label_t& l) const
    {
        return _non_service_label.count(l) == 0;
    }

    void Network::write_prex_routing(std::ostream& s)
    {
        s << "<routes>\n";
        s << "  <routings>\n";
        for(auto& r : _routers)
        {
            if(r->is_null()) continue;
            
            // empty-check
            bool all_empty = true;
            for(auto& inf : r->interfaces())
                all_empty &= inf->table().empty();
            if(all_empty)
                continue;
            
            
            s << "    <routing for=\"" << r->name() << "\">\n";
            s << "      <destinations>\n";
            
            // make uniformly sorted output, easier for debugging
            std::vector<std::pair<std::string,Interface*>> sinfs;
            for(auto& inf : r->interfaces())
            {
                auto fname = r->interface_name(inf->id());
                sinfs.emplace_back(fname.get(), inf.get());
            }
            std::sort(sinfs.begin(), sinfs.end(), [](auto& a, auto& b){
                return strcmp(a.first.c_str(), b.first.c_str()) < 0;
            });
            for(auto& sinf : sinfs)
            {
                auto inf = sinf.second;
                assert(std::is_sorted(inf->table().entries().begin(), inf->table().entries().end()));
                
                for(auto& e : inf->table().entries())
                {
                    assert(e._ingoing == nullptr || e._ingoing == inf);
                    s << "        <destination from=\"" << sinf.first << "\" ";
                    if((e._top_label.type() & Query::MPLS) != 0)
                    {
                        s << "label=\"";
                    }
                    else if((e._top_label.type() & (Query::IP4 | Query::IP6 | Query::ANYIP)) != 0)
                    {
                        s << "ip=\"";
                    }
                    else
                    {
                        assert(false);
                    }
                    s << e._top_label << "\">\n";
                    s << "          <te-groups>\n";
                    s << "            <te-group>\n";
                    s << "              <routes>\n";
                    auto cpy = e._rules;
                    std::sort(cpy.begin(), cpy.end(), [](auto& a, auto& b) {
                        return a._weight < b._weight;
                    });
                    auto ow = cpy.empty() ? 0 : cpy.front()._weight;                    
                    for(auto& rule : cpy)
                    {
                        if(rule._weight > ow)
                        {
                            s << "              </routes>\n";
                            s << "            </te-group>\n";
                            s << "            <te-group>\n";
                            s << "              <routes>\n";
                            ow = rule._weight;
                        }
                        assert(ow == rule._weight);
                        bool handled = false;
                        switch(rule._type)
                        {
                        case RoutingTable::DISCARD:
                            s << "                <discard/>\n";
                            handled = true;
                            break;
                        case RoutingTable::RECEIVE:
                            s << "                <receive/>\n";
                            handled = true;
                            break;
                        case RoutingTable::ROUTE:
                            s << "                <reroute/>\n";
                            handled = true;
                            break;
                        default:
                            break;
                        }
                        if(!handled)
                        {
                            assert(rule._via->source() == r.get());
                            auto tname = r->interface_name(rule._via->id());
                            s << "                <route to=\"" << tname.get() << "\">\n";
                            s << "                  <actions>\n";
                            for(auto& o : rule._ops)
                            {
                                switch(o._op)
                                {
                                case RoutingTable::PUSH:
                                    s << "                    <action arg=\"" << o._op_label << "\" type=\"push\"/>\n";
                                    break;
                                case RoutingTable::POP:
                                    s << "                    <action type=\"pop\"/>\n";
                                    break;
                                case RoutingTable::SWAP:
                                    s << "                    <action arg=\"" << o._op_label << "\" type=\"swap\"/>\n";
                                    break;
                                default:
                                    throw base_error("Unknown op-type");
                                }
                            }
                            s << "                  </actions>\n";
                            s << "                </route>\n";
                        }
                        else
                        {
                            break;
                        }
                    }
                    s << "              </routes>\n";
                    s << "            </te-group>\n";
                    s << "          </te-groups>\n";
                    s << "        </destination>\n";
                }
            }
            s << "      </destinations>\n";
            s << "    </routing>\n";
        }
        s << "  </routings>\n";
        s << "</routes>\n";
    }

    void Network::write_prex_topology(std::ostream& s)
    {
        s << "<network>\n  <routers>\n";
        for(auto& r : _routers)
        {
            if(r->is_null()) continue;
            if(r->interfaces().empty()) continue;
            s << "    <router name=\"" << r->name() << "\">\n";
            s << "      <interfaces>\n";
            for(auto& inf : r->interfaces())
            {
                auto fname = r->interface_name(inf->id());
                s << "        <interface name=\"" << fname.get() << "\"/>\n";
            }
            s << "      </interfaces>\n";
            s << "    </router>\n";
        }
        s << "  </routers>\n  <links>\n";
        for(auto& r : _routers)
        {
            if(r->is_null()) continue;
            for(auto& inf : r->interfaces())
            {
                if(inf->source()->index() > inf->target()->index())
                    continue;
                
                if(inf->source()->is_null()) continue;
                if(inf->target()->is_null()) continue;
                
                auto fname = r->interface_name(inf->id());
                auto oname = inf->target()->interface_name(inf->match()->id());
                s << "    <link>\n      <sides>\n" <<
                     "        <shared_interface interface=\"" << fname.get() <<
                     "\" router=\"" << inf->source()->name() << "\"/>\n" <<
                     "        <shared_interface interface=\"" << oname.get() <<
                     "\" router=\"" << inf->target()->name() << "\"/>\n" <<
                     "      </sides>\n    </link>\n";
            }
        }
        s << "  </links>\n</network>\n";
    }

    Network Network::make_network(const std::vector<std::string>& names, const std::vector<std::vector<std::string>>& links){
        std::vector<std::unique_ptr<Router>> routers;
        std::vector<const Interface*> interfaces;
        Network::routermap_t mapping;
        for (size_t i = 0; i < names.size(); ++i) {
            auto name = names[i];
            size_t id = routers.size();
            routers.emplace_back(std::make_unique<Router>(id));
            Router& router = *routers.back().get();
            router.add_name(name);
            auto res = mapping.insert(name.c_str(), name.length());
            assert(res.first);
            mapping.get_data(res.second) = &router;
            router.get_interface(interfaces, "i" + name);
            for (const auto& other : links[i]) {
                router.get_interface(interfaces, other);
            }
        }
        for (size_t i = 0; i < names.size(); ++i) {
            auto name = names[i];
            for (const auto &other : links[i]) {
                auto res1 = mapping.exists(name.c_str(), name.length());
                assert(res1.first);
                auto res2 = mapping.exists(other.c_str(), other.length());
                if(!res2.first) continue;
                mapping.get_data(res1.second)->find_interface(other)->make_pairing(mapping.get_data(res2.second)->find_interface(name));
            }
        }
        Router::add_null_router(routers, interfaces, mapping);

        return Network(std::move(mapping), std::move(routers), std::move(interfaces));
    }

}