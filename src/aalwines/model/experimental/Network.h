//
// Created by Morten on 05-08-2020.
//

#ifndef AALWINES_NETWORK_H
#define AALWINES_NETWORK_H

#include <aalwines/model/Query.h>
#include <aalwines/model/filter.h>
#include <ptrie/ptrie_map.h>
#include <vector>
#include <memory>
#include <functional>

#include <aalwines/model/experimental/Router.h>
#include <aalwines/model/experimental/RoutingTable.h>

namespace aalwines { namespace experimental {

    class Network {
    public:
        using routermap_t = ptrie::map<char, Router*>;
        Network(routermap_t&& mapping, std::vector<std::unique_ptr < Router>>&& routers, std::vector<const Interface*>&& all_interfaces)
        : _mapping(std::move(mapping)), _routers(std::move(routers)), _all_interfaces(std::move(all_interfaces)) {
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

    private:
        // NO TOUCHEE AFTER INIT!
        routermap_t _mapping;
        std::vector<std::unique_ptr<Router>> _routers;
        size_t _total_labels = 0;
        std::vector<const Interface*> _all_interfaces;
    };
}}


#endif //AALWINES_NETWORK_H
