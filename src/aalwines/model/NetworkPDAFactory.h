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
 * File:   NetworkPDAFactory.h
 * Author: Peter G. Jensen <root@petergjoel.dk>
 *
 * Created on July 19, 2019, 6:26 PM
 */

#ifndef NETWORKPDA_H
#define NETWORKPDA_H

#include <aalwines/model/Query.h>
#include <aalwines/model/Network.h>
#include <pdaaal/PDAFactory.h>
#include <aalwines/model/NetworkTranslation.h>

namespace aalwines {

    template<typename W_FN = std::function<void(void)>, typename W = void>
    class NetworkPDAFactory : public pdaaal::TypedPDAFactory<Query::label_t, W> {
        using label_t = Query::label_t;
        using NFA = pdaaal::NFA<label_t>;
        using weight_type = typename W_FN::result_type;
        static constexpr bool is_weighted = pdaaal::is_weighted<weight_type>;
        using PDAFactory = pdaaal::TypedPDAFactory<label_t, weight_type>;
        using PDA = pdaaal::TypedPDA<label_t>;
        using rule_t = typename PDAFactory::rule_t;
        using nfa_state_t = NFA::state_t;
    private:
        using Translation = NetworkTranslationW<W_FN>;
        using op_t = std::tuple<pdaaal::op_t,label_t>;
        using ops_t = std::vector<op_t>;
        using state_t = std::tuple<const Interface*, const nfa_state_t*, ops_t>;
    public:
        NetworkPDAFactory(const Query& query, Network &network, Builder::labelset_t&& all_labels)
        : NetworkPDAFactory(query, network, std::move(all_labels), [](){}) {};

        NetworkPDAFactory(const Query& query, const Network& network, Builder::labelset_t&& all_labels, const W_FN& weight_f)
        : PDAFactory(std::move(all_labels)), _translation(query, network, weight_f), _query(query), _weight_f(weight_f) { };

        json get_json_trace(std::vector<PDA::tracestate_t> &trace) {
            std::vector<const RoutingTable::entry_t *> entries;
            std::vector<const RoutingTable::forward_t *> rules;
            if (!concreterize_trace(trace, entries, rules)) {
                return json();
            }
            // Do the printing
            return write_concrete_trace(trace, entries, rules);
        }

    protected:
        void build_pda() override {
            _translation.make_initial_states([this](const Interface* inf, const nfa_state_t* nfa_state){
                add_state<true>(state_t(inf, nfa_state, ops_t()));
            });
            for (size_t from_state = 0; from_state < _states.size(); ++from_state) {
                auto [inf, nfa_state, ops] = _states.at(from_state);
                if (ops.empty()) {
                    for (const auto& entry : inf->table()->entries()) {
                        for (const auto& forward : entry._rules) {
                            if (forward._priority > _query.number_of_failures()) continue; // TODO: Approximation here.
                            auto first_op = forward.first_action();
                            ops_t other_ops;
                            for (auto action_it = forward._ops.begin() + 1; action_it != forward._ops.end(); ++action_it) {
                                other_ops.emplace_back(action_it->convert_to_pda_op());
                            }
                            auto apply_per_nfastate = [&,this](const nfa_state_t* nfa_state) {
                                auto to_state = add_state({forward._via->match(), nfa_state, other_ops});
                                rule_t rule{from_state, entry._top_label, to_state, std::get<0>(first_op), std::get<1>(first_op)};
                                if (entry.ignores_label()) {
                                    this->add_wildcard_rule(rule);
                                } else {
                                    this->add_rule(rule);
                                }
                            };
                            if (forward._via->is_virtual()) { // Virtual interface does not use a link, so keep same NFA state.
                                apply_per_nfastate(nfa_state);
                            } else { // Follow NFA edges matching forward._via
                                for (const auto& e : nfa_state->_edges) {
                                    if (!e.contains(forward._via->global_id())) continue;
                                    for (const auto& n : e.follow_epsilon()) {
                                        apply_per_nfastate(n);
                                    }
                                }
                            }
                        }
                    }
                } else {
                    auto first_op = ops[0];
                    auto to_state = _states.insert({inf, nfa_state, ops_t(ops.begin()+1, ops.end())}).second;
                    this->add_wildcard_rule(rule_t{from_state, Query::wildcard_label(), to_state, std::get<0>(first_op), std::get<1>(first_op)});
                }
            }
            assert(std::is_sorted(_initial.begin(), _initial.end()));
            assert(std::is_sorted(_accepting.begin(), _accepting.end()));
        }

        const std::vector<size_t>& initial() override { return _initial; }
        const std::vector<size_t>& accepting() override { return _accepting; }

    private:
        // Construction (factory)
        template<bool initial = false>
        size_t add_state(const state_t& state) {
            auto res = _states.insert(state);
            if (res.first) {
                if (accepting(state)) {
                    _accepting.push_back(res.second);
                }
                if constexpr (initial) {
                    _initial.push_back(res.second);
                }
            }
            return res.second;
        }
        static bool accepting(const state_t& state) {
            return std::get<2>(state).empty() && !std::get<0>(state)->is_virtual() && std::get<1>(state)->_accepting;
        }

        // Trace reconstruction
        bool add_interfaces(std::unordered_set<const Interface* >& disabled, std::unordered_set<const Interface*>& active,
                            const RoutingTable::entry_t& entry, const RoutingTable::forward_t& forward) const {
            if (disabled.count(forward._via) > 0) return false; // should be down!

            // find all rules with a "covered" pre but lower weight.
            // these must have been disabled!
            if (forward._priority == 0) return true;
            std::unordered_set<const Interface *> tmp = disabled;

            // find failing rule and disable _via interface
            for(const auto& rule : entry._rules){
                if (rule._priority < forward._priority) {
                    // This rule must be failing to use forward.
                    if (active.count(rule._via) > 0) return false;
                    tmp.insert(rule._via);
                    if (tmp.size() > _query.number_of_failures()) return false;
                }
            }
            if (tmp.size() > _query.number_of_failures()) {
                return false;
            } else {
                disabled.swap(tmp);
                active.insert(forward._via);
                return true;
            }
        }

        auto get_entries_matching(const label_t& label, const RoutingTable* table) const {
            assert(std::is_sorted(table->entries().begin(), table->entries().end()));
            std::vector<const RoutingTable::entry_t*> matching_entries;
            auto lb = std::lower_bound(table->entries().begin(), table->entries().end(), label, RoutingTable::CompEntryLabel());
            if (lb != table->entries().end() && lb->_top_label == label) {
                matching_entries.emplace_back(&(*lb));
            }
            if (!table->entries().empty() && table->entries().back().ignores_label() && (matching_entries.empty() || !matching_entries.back()->ignores_label())) {
                matching_entries.emplace_back(&table->entries().back());
            }
            return matching_entries;
        }
        bool concreterize_trace(const std::vector<PDA::tracestate_t>& trace, std::vector<const RoutingTable::entry_t*>& entries, std::vector<const RoutingTable::forward_t*>& rules) {
            std::unordered_set<const Interface *> disabled, active;

            for (size_t sno = 0; sno < trace.size(); ++sno) {
                const auto& step = trace[sno];
                auto [inf, nfa_state, ops] = _states.at(step._pdastate);
                if (!ops.empty()) continue;
                if (sno == trace.size() - 1 || step._stack.empty()) continue;
                auto [next_inf, next_nfa_state, next_ops] = _states.at(trace[sno + 1]._pdastate);
                const auto& next_stack = trace[sno + 1]._stack;
                bool found = false;
                for (const RoutingTable::entry_t* entry : get_entries_matching(step._stack.front(), inf->table())) {
                    assert(entry->_top_label == step._stack.front() || entry->ignores_label()); // matching on pre
                    for (const auto& forward : entry->_rules) {
                        if (forward._via->match() != next_inf) continue;

                        bool approximation_ok = false;
                        switch (_query.approximation()) {
                            case Query::mode_t::OVER:
                                approximation_ok = forward._priority <= _query.number_of_failures();
                                break;
                            case Query::mode_t::EXACT:
                                throw base_error("Tracing for EXACT not yet implemented");
                        }
                        if (!approximation_ok) continue;

                        auto expected_next_stack_size = step._stack.size();
                        bool top_label_ok = true;
                        if (forward._ops.empty()) {
                            top_label_ok = step._stack.front() == next_stack.front();
                        } else {
                            switch (forward._ops[0]._op) {
                                case RoutingTable::op_t::POP:
                                    expected_next_stack_size--;
                                    break;
                                case RoutingTable::op_t::PUSH:
                                    expected_next_stack_size++;
                                case RoutingTable::op_t::SWAP:
                                    top_label_ok = forward._ops[0]._op_label == next_stack.front();
                                    break;
                            }
                        }
                        if (expected_next_stack_size == next_stack.size() && top_label_ok) {
                            if (!add_interfaces(disabled, active, *entry, forward)) continue;
                            rules.push_back(&forward);
                            entries.push_back(entry);
                            found = true;
                            break;
                        }
                    }
                    if (found) break;
                }
                // check if we violate the soundness of the network
                if (!found) {
                    assert(false);
                    return false;
                }

            }
            return true;
        }

        json write_concrete_trace(const std::vector<PDA::tracestate_t>& trace, std::vector<const RoutingTable::entry_t*>& entries, std::vector<const RoutingTable::forward_t*>& rules) {
            auto result_trace = json::array();
            size_t cnt = 0;
            for (const auto &step : trace) {
                auto [inf, nfa_state, ops] = _states.at(step._pdastate);
                if (ops.empty()) {
                    Translation::add_link_to_trace(result_trace, inf, step._stack);
                    if (cnt < entries.size()) {
                        _translation.add_rule_to_trace(result_trace, inf, *entries[cnt], *rules[cnt]);
                        ++cnt;
                    }
                }
            }
            return result_trace;
        }

        Translation _translation;
        const Query& _query;
        pdaaal::ptrie_set<state_t> _states;
        std::vector<size_t> _initial;
        std::vector<size_t> _accepting;
        const W_FN& _weight_f;
    };

    template<typename W_FN>
    NetworkPDAFactory(Query& query, Network& network, Builder::labelset_t&& all_labels, const W_FN& weight_f) -> NetworkPDAFactory<W_FN, typename W_FN::result_type>;

}

#endif /* NETWORKPDA_H */

