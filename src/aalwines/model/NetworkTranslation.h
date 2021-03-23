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

namespace aalwines {

    /*
    struct State {
        using nfa_state_t = typename pdaaal::NFA<Query::label_t>::state_t;
        size_t _eid = 0; // which entry we are going for
        size_t _rid = 0; // which rule in that entry
        size_t _opid = std::numeric_limits<size_t>::max(); // which operation is the first in the rule (max = no operations left).
        size_t _appmode = 0; // mode of approximation
        const Interface *_inf = nullptr;
        const nfa_state_t* _nfa_state = nullptr;
        State() = default;
        State(size_t eid, size_t rid, size_t opid, size_t appmode, const Interface* inf, const nfa_state_t* nfa_state)
                : _eid(eid), _rid(rid), _opid(opid), _appmode(appmode), _inf(inf), _nfa_state(nfa_state) { };
        State(const Interface* inf, const nfa_state_t* nfa_state, size_t appmode = 0)
                : _appmode(appmode), _inf(inf), _nfa_state(nfa_state) { };
        bool operator==(const State& other) const {
            return _eid == other._eid && _rid == other._rid && _opid == other._opid
                   && _appmode == other._appmode && _inf == other._inf && _nfa_state == other._nfa_state;
        }
        bool operator!=(const State& other) const {
            return !(*this == other);
        }
        [[nodiscard]] const Interface* interface() const {
            return _inf;
        }
        [[nodiscard]] bool ops_done() const {
            return _opid == std::numeric_limits<size_t>::max();
        }
        [[nodiscard]] bool accepting() const {
            return ops_done() && (_inf == nullptr || !_inf->is_virtual()) && _nfa_state->_accepting;
        }
        static State perform_op(const State& state) {
            assert(!state.ops_done());
            return perform_op(state, state._inf->table()->entries()[state._eid]._rules[state._rid]);
        }
        static State perform_op(const State& state, const RoutingTable::forward_t& forward) {
            assert(!state.ops_done());
            if (state._opid + 2 == forward._ops.size()) {
                return State(forward._via->match(), state._nfa_state, state._appmode);
            } else {
                return State(state._eid, state._rid, state._opid + 1, state._appmode, state._inf, state._nfa_state);
            }
        }
    } __attribute__((packed)); // packed is needed to make this work fast with ptries
    */

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

        /*
        void rules(const State& from_state,
                          const std::function<void(State&&, const RoutingTable::entry_t&, const RoutingTable::forward_t&)>& add_rule_type_a,
                          const std::function<void(State&&, const label_t&, std::pair<pdaaal::op_t,label_t>&&)>& add_rule_type_b) {
            if (from_state.ops_done()) {
                const auto& entries = from_state._inf->table()->entries();
                for (size_t eid = 0; eid < entries.size(); ++eid) {
                    const auto& entry = entries[eid];
                    for (size_t rid = 0; rid < entry._rules.size(); ++rid) {
                        const auto& forward = entry._rules[rid];
                        auto appmode = set_approximation(from_state, forward);
                        if (appmode == std::numeric_limits<size_t>::max()) continue;
                        auto apply_per_nfastate = [&](const nfa_state_t* n) {
                            add_rule_type_a((forward._ops.size() <= 1) ? State(forward._via->match(), n, appmode) : State(eid, rid, 0, appmode, from_state._inf, n), entry, forward);
                        };
                        if (forward._via->is_virtual()) { // Virtual interface does not use a link, so keep same NFA state.
                            apply_per_nfastate(from_state._nfa_state);
                        } else { // Follow NFA edges matching forward._via
                            for (const auto& e : from_state._nfa_state->_edges) {
                                if (!e.contains(forward._via->global_id())) continue;
                                for (const auto& n : e.follow_epsilon()) {
                                    apply_per_nfastate(n);
                                }
                            }
                        }

                    }
                }
            } else {
                const auto& forward = from_state._inf->table()->entries()[from_state._eid]._rules[from_state._rid];
                assert(from_state._opid + 1 < forward._ops.size()); // Otherwise we would already have moved to the next interface.

                auto pre_label = (forward._ops[from_state._opid]._op == RoutingTable::op_t::POP)
                        ? Query::wildcard_label()
                        : forward._ops[from_state._opid]._op_label;
                add_rule_type_b(State::perform_op(from_state, forward), // To-state
                                pre_label,
                                forward._ops[from_state._opid + 1].convert_to_pda_op()); // Op, op-label
            }
        }*/

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
                std::vector<const Interface*> out_infs, temp;
                for (const auto& table : interface->target()->tables()) {
                    auto lb = std::lower_bound(table->out_interfaces().begin(), table->out_interfaces().end(), interface->match());
                    if (lb != table->out_interfaces().end() && *lb == interface->match()) {
                        std::set_union(out_infs.begin(), out_infs.end(), table->out_interfaces().begin(), table->out_interfaces().end(), std::back_inserter(temp));
                        std::swap(temp, out_infs);
                    }
                }
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

        /*
        size_t set_approximation(const State& state, const RoutingTable::forward_t& forward) {
            if (forward._via->is_virtual()) return state._appmode;
            auto num_fail = _query.number_of_failures();
            auto err = std::numeric_limits<size_t>::max();
            switch (_query.approximation()) {
                case Query::mode_t::OVER:
                    return (forward._priority > num_fail) ? err : 0; // TODO: This is incorrect. Should be size of set of links for forwards with smaller priority on current entry...
                case Query::mode_t::EXACT:
                default:
                    return err;
            }
        }*/

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
