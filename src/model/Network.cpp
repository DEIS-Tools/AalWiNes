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
 * File:   Network.cpp
 * Author: Peter G. Jensen <root@petergjoel.dk>
 * 
 * Created on July 17, 2019, 2:17 PM
 */

#include <unistd.h>

#include "Network.h"
#include "filter.h"

namespace mpls2pda {

    Network::Network(ptrie::map<Router*>&& mapping, std::vector<std::unique_ptr<Router> >&& routers)
    : _mapping(std::move(mapping)), _routers(std::move(routers))
    {
        for(auto& r : _routers)
        {
            for(auto& t : r->tables())
            {
                for(auto& e : t.entries())
                {
                    _label_map[e._top_label].emplace_back(&e, r.get());
                }
            }
        }
    }
    
    const Router* Network::get_router(size_t id) const
    {
        if(id < _routers.size())
        {
            return _routers[id].get();
        }
        else
        {
            return nullptr;
        }
    }
    
    const char* empty_string = "";
    std::unordered_set<Query::label_t> Network::interfaces(filter_t& filter)
    {
        std::unordered_set<Query::label_t> res;
        for(auto& r : _routers)
        {
            if(filter._from(r->name().c_str()))
            {
                for(auto& i : r->interfaces())
                {
                    if(i->is_virtual()) continue;
                    // can we have empty interfaces??
                    assert(i);
                    auto fname = r->interface_name(i->id());
                    auto fip = i->ip4();
                    auto fip6 = i->ip6();
                    std::unique_ptr<char[]> tname = std::make_unique<char[]>(1);
                    tname.get()[0] = '\0';
                    const char* tr = empty_string;
                    uint32_t tip = 0;
                    uint64_t tip6 = 0;
                    if(i->target() != nullptr)
                    {
                        if(i->match() != nullptr)
                        {
                            tname = i->target()->interface_name(i->match()->id());
                            tip = i->match()->ip4();
                            tip6 = i->match()->ip6();
                        }
                        tr = i->target()->name().c_str();
                    }
                    if(filter._link(fname.get(), fip, fip6, tname.get(), tip, tip6, tr))
                    {
                        res.insert(reinterpret_cast<size_t>(i.get())); // TODO: little hacksy, but we have uniform types in the parser
                    }
                }
            }
        }
        return res;
    }

    std::unordered_set<Query::label_t> Network::get_labels(uint64_t label, uint64_t mask)
    {
        std::unordered_set<Query::label_t> res;
        for(auto& pr : _label_map)
        {
            for(auto& entry : pr.second)
            {
                if(entry.first->is_mpls())
                {
                    auto mpls = entry.first->as_mpls();
                    if((mpls / mask) == (label / mask))
                        res.insert(reinterpret_cast<ssize_t>(entry.first->_top_label));
                }
            }
        }
        return res;
    }

    std::unordered_set<Query::label_t> Network::ip_labels(filter_t& filter)
    {
        // TODO: we already know the routers form the first matching, so we could just iterate over these.
        // possibly same thing if using interfaces instead.
        std::unordered_set<Query::label_t> res;
        auto ifaces = interfaces(filter);
        for(auto& pr : _label_map)
        {
            for(auto& entry : pr.second)
            {
                if(entry.first->is_interface())
                {
                    auto iid = entry.first->as_interface();
                    Interface* inf = entry.second->interface_no(iid);
                    if(inf->is_virtual()) continue;
                    if(ifaces.count(reinterpret_cast<ssize_t>(inf)) > 0)
                        res.insert(reinterpret_cast<ssize_t>(entry.first->_top_label));
                }
            }
        }
        return res;
    }
    
    void Network::print_dot(std::ostream& s)
    {
        s << "digraph network {\n";
        for (auto& r : _routers) {
            r->print_dot(s);
        }
        s << "}" << std::endl;
    }

    void Network::print_json(std::ostream& s)
    {
        s << "{\n";
        for (size_t i = 0; i < _routers.size(); ++i) {
            if (i != 0)
                s << ",\n";
            s << "\"" << _routers[i]->name() << "\":";
            _routers[i]->print_json(s);
        }
        s << "\n}\n";
    }

}