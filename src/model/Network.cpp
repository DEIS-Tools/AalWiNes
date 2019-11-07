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

#include <unistd.h>

#include "Network.h"
#include "filter.h"

namespace mpls2pda
{

    Network::Network(ptrie::map<Router*>&& mapping, std::vector<std::unique_ptr<Router> >&& routers, std::vector<const Interface*>&& all_interfaces)
    : _mapping(std::move(mapping)), _routers(std::move(routers)), _all_interfaces(std::move(all_interfaces))
    {
        for (auto& r : _routers) {
            for (auto& inf : r->interfaces()) {
                for (auto& e : inf->table().entries()) {
                    _label_map[e._top_label].emplace_back(&e, r.get());
                }
            }
        }
    }

    const Router* Network::get_router(size_t id) const
    {
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

    std::unordered_set<Query::label_t> Network::get_labels(uint64_t label, uint64_t mask, Query::type_t type)
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
                throw base_error("Unknown exapnsion");
            }
        }
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
            throw base_error("Unknown exapnsion");
        }        
        return res;
    }

    std::unordered_set<Query::label_t> Network::all_labels()
    {
        if(_label_cache.size() == 0)
        {
            std::unordered_set<Query::label_t> res;
            res.reserve(_label_map.size() + 7);
            res.insert(Query::label_t::unused_ip4);
            res.insert(Query::label_t::unused_ip6);
            res.insert(Query::label_t::unused_mpls);
            res.insert(Query::label_t::unused_sticky_mpls);
            res.insert(Query::label_t::any_ip);
            res.insert(Query::label_t::any_ip4);
            res.insert(Query::label_t::any_ip6);
            res.insert(Query::label_t::any_mpls);
            res.insert(Query::label_t::any_sticky_mpls);
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

    
}