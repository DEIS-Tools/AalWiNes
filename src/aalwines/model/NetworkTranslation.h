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
 * File:   NetworkTranslation.h
 * Author: Morten K. Schou <morten@h-schou.dk>
 *
 * Created on 11-02-2021.
 */

#ifndef AALWINES_NETWORKTRANSLATION_H
#define AALWINES_NETWORKTRANSLATION_H

#include <aalwines/model/Query.h>
#include <aalwines/model/Network.h>
#include <aalwines/utils/more_algorithms.h>

namespace aalwines {

    // This one is general, and will be reused by both NetworkPDAFactory and CegarNetworkPDAFactory
    class NetworkTranslation {
    protected:
        using label_t = Query::label_t;
        using NFA = pdaaal::NFA<label_t>;
        using nfa_state_t = NFA::state_t;
    public:
        NetworkTranslation(const Query& query, const Network& network) : _query(query), _network(network) { };

        void make_initial_states(const std::function<void(const Interface*, const nfa_state_t*)>& add) {
            auto add_many = [&add](const Interface* inf, const std::vector<nfa_state_t*>& next) {
                for (const auto& nfa_state : next) {
                    add(inf, nfa_state);
                }
            };
            make_initial_states(add_many);
        }
        void make_initial_states(const std::function<void(const Interface*, const std::vector<nfa_state_t*>&)>& add) {
            for (const auto& i : _query.path().initial()) {
                for (const auto& e : i->_edges) {
                    auto next = e.follow_epsilon();
                    if (e.wildcard(_network.all_interfaces().size())) {
                        for (const auto& inf : _network.all_interfaces()) {
                            if (!inf->is_virtual()) {
                                add(inf->match(), next);
                            }
                        }
                    } else if (!e._negated) {
                        for (const auto& s : e._symbols) {
                            auto inf = _network.all_interfaces()[s];
                            if (!inf->is_virtual()) {
                                add(inf->match(), next);
                            }
                        }
                    } else {
                        for (const auto& inf : _network.all_interfaces()) {
                            if (e.contains(inf->global_id()) && !inf->is_virtual()) {
                                add(inf->match(), next);
                            }
                        }
                    }
                }
            }
        }

        using edge_variant = std::variant<const RoutingTable*, const Interface*, const Interface*>;
        static edge_variant special_interface(const Interface* interface) {
            return edge_variant(std::in_place_index<2>, interface);
        }
        template<bool initial=false>
        static edge_variant get_edge_pointer(const Interface* interface) {
            if constexpr (initial) { // Special case for initial states, where we don't have a table from previous state.
                if (interface->table()->interfaces().size() == 1) {
                    assert(interface->table()->interfaces()[0] == interface);
                    return interface->table();
                } else {
                    return edge_variant(std::in_place_index<1>, interface);
                }
            } else {
                auto out_infs = utils::flat_union_if(interface->target()->tables(),
                    [](const auto& table){ return table->out_interfaces(); },
                    [match=interface->match()](const auto& table){ return utils::sorted_contains(table->out_interfaces(), match); }
                );
                auto intersection = interface_intersection(out_infs, interface->table());
                if (intersection.size() == 1) {
                    assert(intersection[0] == interface);
                    return interface->table(); // interface is uniquely identified by interface->table() and any table t with interface->match() in t->out_interfaces() (t being part of previous state in PDA.)
                } else {
                    assert(intersection.size() > 1);
                    return edge_variant(std::in_place_index<1>, interface); // Multiple edges (inf,inf->match()) pairs correspond to the same pair of tables, so we need interface to identify edge.
                }
            }
        }
        static std::vector<const Interface*> interface_intersection(const RoutingTable* from, const RoutingTable* to) {
            assert(from != nullptr);
            return interface_intersection(from->out_interfaces(), to);
        }
        static std::vector<const Interface*> interface_intersection(const std::vector<const Interface*>& from_out_infs, const RoutingTable* to) {
            assert(to != nullptr);
            std::vector<const Interface*> from_out_match_infs;
            std::transform(from_out_infs.begin(), from_out_infs.end(),
                           std::back_inserter(from_out_match_infs), [](const auto& inf){ return inf->match(); });
            std::sort(from_out_match_infs.begin(), from_out_match_infs.end());
            std::vector<const Interface*> intersection;
            std::set_intersection(to->interfaces().begin(), to->interfaces().end(),
                                  from_out_match_infs.begin(), from_out_match_infs.end(),
                                  std::back_inserter(intersection));
            return intersection;
        }
        static const RoutingTable* get_table(const edge_variant& variant) {
            switch (variant.index()) {
                case 0:
                    return std::get<0>(variant);
                case 1:
                    return std::get<1>(variant)->table();
                case 2:
                default:
                    return std::get<2>(variant)->table();
            }
        }
        static const Interface* get_interface(const edge_variant& variant, const RoutingTable* from = nullptr) {
            switch (variant.index()) {
                case 0:
                    return get_interface(from, std::get<0>(variant));
                case 1:
                    return std::get<1>(variant);
                case 2:
                default:
                    return std::get<2>(variant);
            }
        }
        static const Interface* get_interface(const RoutingTable* from, const RoutingTable* to) {
            assert(to != nullptr);
            if (from == nullptr) {
                assert(to->interfaces().size() == 1);
                return to->interfaces()[0];
            } else {
                auto intersection = interface_intersection(from, to);
                assert(intersection.size() == 1);
                return intersection[0];
            }
        }

        static void add_link_to_trace(json& trace, const Interface* inf, const std::vector<label_t>& final_header) {
            trace.emplace_back();
            trace.back()["from_router"] = inf->target()->name();
            trace.back()["from_interface"] = inf->match()->get_name();
            trace.back()["to_router"] = inf->source()->name();
            trace.back()["to_interface"] = inf->get_name();
            trace.back()["stack"] = json::array();
            std::for_each(final_header.rbegin(), final_header.rend(), [&stack=trace.back()["stack"]](const auto& label){
                if (label == Query::bottom_of_stack()) return;
                std::stringstream s;
                s << label;
                stack.emplace_back(s.str());
            });
        }

    private:
        const Query& _query;
        const Network& _network;
    };

    template<typename W_FN = std::function<void(void)>>
    class NetworkTranslationW : public NetworkTranslation {
        using weight_type = typename W_FN::result_type;
        static constexpr bool is_weighted = pdaaal::is_weighted<weight_type>;
    public:
        NetworkTranslationW(const Query& query, const Network& network, const W_FN& weight_f)
        : NetworkTranslation(query, network), _weight_f(weight_f) { };

        void add_rule_to_trace(json& trace, const Interface* inf, const RoutingTable::entry_t& entry, const RoutingTable::forward_t& rule) const {
            trace.emplace_back();
            trace.back()["ingoing"] = inf->get_name();
            std::stringstream s;
            if (entry.ignores_label()) {
                s << "null";
            } else {
                s << entry._top_label;
            }
            trace.back()["pre"] = s.str();
            trace.back()["rule"] = rule.to_json();
            if constexpr (is_weighted) {
                trace.back()["priority-weight"] = json::array();
                auto weights = _weight_f(rule, entry);
                for (const auto& w : weights){
                    trace.back()["priority-weight"].push_back(std::to_string(w));
                }
            }
        }

    private:
        const W_FN& _weight_f;
    };

}

#endif //AALWINES_NETWORKTRANSLATION_H
