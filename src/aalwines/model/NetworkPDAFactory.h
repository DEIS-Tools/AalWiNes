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
#include <pdaaal/NewPDAFactory.h>
#include <aalwines/model/NetworkTranslation.h>

namespace aalwines {

    template<typename W_FN = std::function<void(void)>, typename W = void>
    class NetworkPDAFactory : public pdaaal::NewPDAFactory<Query::label_t, W> {
        using label_t = Query::label_t;
        using NFA = pdaaal::NFA<label_t>;
        using weight_type = typename W_FN::result_type;
        static constexpr bool is_weighted = pdaaal::is_weighted<weight_type>;
        using PDAFactory = pdaaal::NewPDAFactory<label_t, weight_type>;
        using PDA = pdaaal::TypedPDA<label_t>;
        using rule_t = typename PDAFactory::rule_t;
        using nfa_state_t = NFA::state_t;
    private:
        using Translation = NetworkTranslationW<W_FN>;
        using state_t = State;
    public:
        NetworkPDAFactory(Query &query, Network &network, Builder::labelset_t&& all_labels)
        : NetworkPDAFactory(query, network, std::move(all_labels), [](){}) {};

        NetworkPDAFactory(Query &query, Network &network, Builder::labelset_t&& all_labels, const W_FN& weight_f)
        : PDAFactory(std::move(all_labels)), _translation(query, weight_f), _network(network), _query(query), _weight_f(weight_f) {
            construct_initial();
        };

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
        const std::vector<size_t>& initial() override { return _initial; }

        bool accepting(size_t i) override { return _states.get_data(i); }

        std::vector<rule_t> rules(size_t from_id) override {
            if (_query.approximation() == Query::EXACT ||
                _query.approximation() == Query::DUAL) {
                throw base_error("Exact and Dual analysis method not yet supported");
            }
            state_t from_state;
            _states.unpack(from_id, &from_state);
            std::vector<rule_t> result;

            _translation.rules(from_state,
            [from_id,&result,this](state_t&& to_state, const RoutingTable::entry_t& entry, const RoutingTable::forward_t& forward) {
                rule_t rule;
                rule._from = from_id;
                std::tie(rule._op, rule._op_label) = Translation::first_action(forward);
                rule._to = add_state(to_state);
                rule._pre = entry._top_label;
                if constexpr (is_weighted) {
                    rule._weight = _weight_f(forward, entry);
                }
                result.push_back(rule);
                if (result.back()._pre == Query::wildcard_label()) {
                    expand_back(result); // TODO: Implement wildcard pre label in PDA instead of this.
                }
            },
            [from_id,&result,this](state_t&& to_state, const label_t& pre_label, std::pair<pdaaal::op_t,label_t>&& op) {
                rule_t rule;
                rule._from = from_id;
                std::tie(rule._op, rule._op_label) = op;
                rule._to = add_state(to_state);
                rule._pre = pre_label;
                result.push_back(rule);
                if (result.back()._pre == Query::wildcard_label()) {
                    expand_back(result); // TODO: Implement wildcard pre label in PDA instead of this.
                }
            });

            return result;
        }

    private:
        // Construction (factory)
        template<bool initial = false>
        size_t add_state(const state_t& state) {
            auto res = _states.insert(state);
            if (res.first) {
                _states.get_data(res.second) = state.accepting();
                if constexpr (initial) {
                    _initial.push_back(res.second);
                }
            }
            return res.second;
        }

        void construct_initial() {
            _translation.make_initial_states(_network.all_interfaces(), [this](state_t&& state) {
                add_state<true>(state);
            });
            std::sort(_initial.begin(), _initial.end());
        }
        void expand_back(std::vector<rule_t> &rules) {
            rule_t cpy;
            std::swap(rules.back(), cpy);
            rules.pop_back();
            for (const auto& label : this->_all_labels) {
                rules.push_back(cpy);
                rules.back()._pre = label;
            }
        }

        // Trace reconstruction
        bool add_interfaces(std::unordered_set<const Interface* >& disabled, std::unordered_set<const Interface*>& active,
                            const RoutingTable::entry_t& entry, const RoutingTable::forward_t& forward) const {
            if (disabled.count(forward._via) > 0) {
                return false; // should be down!
            }
            // find all rules with a "covered" pre but lower weight.
            // these must have been disabled!
            if (forward._priority == 0)
                return true;
            std::unordered_set<const Interface *> tmp = disabled;

            // find failing rule and disable _via interface
            for(auto &rule : entry._rules){
                if (rule._priority < forward._priority) {
                    if (active.count(rule._via) > 0) return false;
                    tmp.insert(rule._via);
                    if (tmp.size() > _query.number_of_failures()) {
                        return false;
                    }
                    break;
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



        bool concreterize_trace(const std::vector<PDA::tracestate_t>& trace, std::vector<const RoutingTable::entry_t*>& entries, std::vector<const RoutingTable::forward_t*>& rules) {
            std::unordered_set<const Interface *> disabled, active;

            for (size_t sno = 0; sno < trace.size(); ++sno) {
                auto &step = trace[sno];
                state_t s;
                _states.unpack(step._pdastate, &s);
                if (s.ops_done()) {
                    if (sno != trace.size() - 1 && !step._stack.empty()) {
                        // peek at next element, we want to write the ops here
                        state_t next;
                        _states.unpack(trace[sno + 1]._pdastate, &next);
                        if (!next.ops_done()) {
                            // we get the rule we use, print
                            auto &entry = next._inf->table().entries()[next._eid];
                            if (!add_interfaces(disabled, active, entry, entry._rules[next._rid])) {
                                return false;
                            }
                            rules.push_back(&entry._rules[next._rid]);
                            entries.push_back(&entry);
                        } else {
                            // we have to guess which rule we used!
                            // run through the rules and find a match!
                            auto &nstep = trace[sno + 1];
                            bool found = false;
                            for (auto &entry : s._inf->table().entries()) {
                                if (found) break;
                                if (entry._top_label != step._stack.front())
                                    continue; // not matching on pre
                                for (auto &r : entry._rules) {
                                    bool ok = false;
                                    switch (_query.approximation()) {
                                        case Query::UNDER:
                                            assert(next._appmode >= s._appmode);
                                            ok = r._priority == (next._appmode - s._appmode);
                                            break;
                                        case Query::OVER:
                                            ok = r._priority <= _query.number_of_failures();
                                            break;
                                        case Query::DUAL:
                                            throw base_error("Tracing for DUAL not yet implemented");
                                        case Query::EXACT:
                                            throw base_error("Tracing for EXACT not yet implemented");
                                    }
                                    if (ok) { // TODO, fix for approximations here!
                                        if (r._ops.size() > 1) continue; // would have been handled in other case

                                        if (r._via && r._via->match() == next._inf) {
                                            if (r._ops.empty() || r._ops[0]._op == RoutingTable::op_t::SWAP) {
                                                if (step._stack.size() == nstep._stack.size()) {
                                                    if (!r._ops.empty()) {
                                                        assert(r._ops[0]._op == RoutingTable::op_t::SWAP);
                                                        if (nstep._stack.front() != r._ops[0]._op_label)
                                                            continue;
                                                    } else if (entry._top_label != nstep._stack.front()) {
                                                        continue;
                                                    }
                                                    if (!add_interfaces(disabled, active, entry, r))
                                                        continue;
                                                    rules.push_back(&r);
                                                    entries.push_back(&entry);
                                                    found = true;
                                                    break;
                                                }
                                            } else {
                                                assert(r._ops.size() == 1);
                                                assert(r._ops[0]._op == RoutingTable::op_t::PUSH ||
                                                       r._ops[0]._op == RoutingTable::op_t::POP);
                                                if (r._ops[0]._op == RoutingTable::op_t::POP &&
                                                    nstep._stack.size() == step._stack.size() - 1) {
                                                    if (!add_interfaces(disabled, active, entry, r))
                                                        continue;
                                                    rules.push_back(&r);
                                                    entries.push_back(&entry);
                                                    found = true;
                                                    break;
                                                } else if (r._ops[0]._op == RoutingTable::op_t::PUSH &&
                                                           nstep._stack.size() == step._stack.size() + 1 &&
                                                           r._ops[0]._op_label == nstep._stack.front()) {
                                                    if (!add_interfaces(disabled, active, entry, r))
                                                        continue;

                                                    rules.push_back(&r);
                                                    entries.push_back(&entry);
                                                    found = true;
                                                    break;
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                            // check if we violate the soundness of the network
                            if (!found) {
                                assert(false);
                                return false;
                            }
                        }
                    }
                }
            }
            return true;
        }

        json write_concrete_trace(const std::vector<PDA::tracestate_t>& trace, std::vector<const RoutingTable::entry_t*>& entries, std::vector<const RoutingTable::forward_t*>& rules) {
            auto result_trace = json::array();
            size_t cnt = 0;
            for (const auto &step : trace) {
                state_t s;
                _states.unpack(step._pdastate, &s);
                if (s.ops_done()) {
                    Translation::add_link_to_trace(result_trace, s, step._stack);
                    if (cnt < entries.size()) {
                        _translation.add_rule_to_trace(result_trace, s._inf, *entries[cnt], *rules[cnt]);
                        ++cnt;
                    }
                }
            }
            return result_trace;
        }

        Translation _translation;
        Network& _network;
        Query& _query;
        std::vector<size_t> _initial;
        ptrie::map<state_t,bool> _states;
        const W_FN& _weight_f;
    };

    template<typename W_FN>
    NetworkPDAFactory(Query& query, Network& network, Builder::labelset_t&& all_labels, const W_FN& weight_f) -> NetworkPDAFactory<W_FN, typename W_FN::result_type>;

}

#endif /* NETWORKPDA_H */

