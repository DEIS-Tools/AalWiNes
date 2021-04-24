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
 *  Copyright Morten K. Schou
 */

/* 
 * File:   CegarVerifier.h
 * Author: Morten K. Schou <morten@h-schou.dk>
 *
 * Created on 21-12-2020.
 */

#ifndef AALWINES_CEGARVERIFIER_H
#define AALWINES_CEGARVERIFIER_H

#include "CegarNetworkPdaFactory.h"
#include <pdaaal/Solver.h>

namespace aalwines {

    class CegarVerifier {
    public:
        template<bool no_abstraction = false, bool use_pre_star = false, pdaaal::refinement_option_t refinement_option = pdaaal::refinement_option_t::best_refinement, bool use_dual = false>
        static std::optional<json> verify(const Network &network, Query& query, std::unordered_set<Query::label_t>&& all_labels, json& json_output) {
            query.compile_nfas();
            // TODO: Weights
            if constexpr (no_abstraction) {
                CegarNetworkPdaFactory<> factory(json_output, network, query, std::move(all_labels),
                                                 [](const Query::label_t& label) -> Query::label_t { return label; },
                                                 [](const Interface* inf){ return inf->global_id();});
                pdaaal::CEGAR<CegarNetworkPdaFactory<>,CegarNetworkPdaReconstruction<refinement_option>> cegar;
                auto res = cegar.template cegar_solve<use_pre_star,use_dual>(std::move(factory), query.construction(), query.destruction());
                if (res) return std::move(res).value().get();
                return std::nullopt;
            } else {
                // Identify edges in the path NFA that explicitly mentions an interface. Use this for initial abstraction.
                using edge_t = const typename pdaaal::NFA<Query::label_t>::edge_t*;
                std::unordered_map<const Interface*, std::vector<edge_t>> inf_map;
                for (const auto& state : query.path().states()) {
                    for (const auto& edge : state->_edges) {
                        for (const auto& symbol : edge._symbols) {
                            auto inf = network.all_interfaces()[symbol]->match();
                            inf_map.try_emplace(inf).first->second.emplace_back(&edge);
                        }
                    }
                }
                // TODO: Do the same with labels mentioned in initial and final NFAs.

                // We distinguish labels based on the next hops that it leads to.
                std::unordered_map<Query::label_t, std::unordered_set<const Interface*>> label_to_next_hops;
                for (const auto& router : network.routers()) {
                    for (const auto& table : router->tables()) {
                        for (const auto& entry : table->entries()) {
                            for (const auto& forward : entry._rules) {
                                if (forward._priority > query.number_of_failures()) continue; // TODO: Approximation here.
                                label_to_next_hops.try_emplace(entry._top_label).first->second.emplace(forward._via);
                            }
                        }
                    }
                }
                // This is a bit slow, but it's okay for now..
                std::vector<std::vector<const Interface*>> next_hops_set;
                std::unordered_map<Query::label_t, size_t> label_map;
                for (const auto& [label, nh_set] : label_to_next_hops) {
                    std::vector<const Interface*> next_hops(nh_set.begin(), nh_set.end());
                    std::sort(next_hops.begin(), next_hops.end());
                    auto it = std::find(next_hops_set.begin(), next_hops_set.end(), next_hops);
                    label_map.emplace(label, it - next_hops_set.begin());
                    if (it == next_hops_set.end()) {
                        next_hops_set.emplace_back(std::move(next_hops));
                    }
                }

                CegarNetworkPdaFactory<> factory(json_output, network, query, std::move(all_labels),
                    [&label_map](const auto& label) -> size_t {
                        switch (label) { // Special labels map to distinct values, but all normal labels in the network maps to the same abstract label.
                            case Query::unused_label():
                                return 0;
                            case Query::bottom_of_stack():
                                return 1;
                            default:
                                auto it = label_map.find(label);
                                return (it == label_map.end() ? 2 : it->second + 3);
                        }
                    },
                    [&inf_map](const Interface* inf) -> std::tuple<bool, std::vector<edge_t>> {
                        auto it = inf_map.find(inf);
                        return std::make_tuple(
                                inf->is_virtual(), // Distinguishing virtual interfaces from non-virtual simplifies the abstraction construction.
                             (it == inf_map.end()) ? std::vector<edge_t>() : it->second); // Use the NFA edges that explicitly mentions inf as the 'abstract state', i.e. group together interfaces that are mentioned similarly.
                    }
                );
                pdaaal::CEGAR<CegarNetworkPdaFactory<>,CegarNetworkPdaReconstruction<refinement_option>> cegar;
                auto res = cegar.template cegar_solve<use_pre_star,use_dual>(std::move(factory), query.construction(), query.destruction());
                if (res) return std::move(res).value().get();
                return std::nullopt;
            }
        }

    };

}

#endif //AALWINES_CEGARVERIFIER_H
