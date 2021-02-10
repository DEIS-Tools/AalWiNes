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

#include "Query.h"
#include "Network.h"
#include <pdaaal/NewPDAFactory.h>


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
                return perform_op(state, state._inf->table().entries()[state._eid]._rules[state._rid]);
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
    public:
        NetworkPDAFactory(Query &query, Network &network, Builder::labelset_t&& all_labels)
        : NetworkPDAFactory(query, network, std::move(all_labels), [](){}) {};

        NetworkPDAFactory(Query &query, Network &network, Builder::labelset_t&& all_labels, const W_FN& weight_f)
        : PDAFactory(std::move(all_labels)), _network(network), _query(query), _path(query.path()), _weight_f(weight_f) {
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
            State from_state;
            _states.unpack(from_id, &from_state);
            std::vector<rule_t> result;
            if (from_state.ops_done()) {
                const auto& entries = from_state._inf->table().entries();
                for (size_t eid = 0; eid < entries.size(); ++eid) {
                    const auto& entry = entries[eid];
                    for (size_t rid = 0; rid < entry._rules.size(); ++rid) {
                        const auto& forward = entry._rules[rid];
                        auto appmode = forward._via->is_virtual() ? set_approximation(from_state, forward) : from_state._appmode;
                        if (appmode == std::numeric_limits<size_t>::max()) continue;
                        rule_t rule;
                        rule._from = from_id;
                        std::tie(rule._op, rule._op_label) = first_action(forward);
                        if constexpr (is_weighted) {
                            rule._weight = _weight_f(forward, entry);
                        }
                        auto apply_per_nfastate = [&](const nfa_state_t* n) {
                            rule._to = (forward._ops.size() <= 1)
                                       ? add_state(State(forward._via->match(), n, appmode)).second
                                       : add_state(State(eid, rid, 0, appmode, from_state._inf, n)).second;
                            result.push_back(rule);
                            if (entry.ignores_label()) {
                                expand_back(result); // TODO: Implement wildcard pre label in PDA instead of this.
                            } else {
                                result.back()._pre = entry._top_label;
                            }
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
                const auto& forward = from_state._inf->table().entries()[from_state._eid]._rules[from_state._rid];
                assert(from_state._opid + 1 < forward._ops.size()); // Otherwise we would already have moved to the next interface.
                rule_t rule;
                rule._from = from_id;
                std::tie(rule._op, rule._op_label) = convert_action(forward._ops[from_state._opid + 1]);
                rule._to = add_state(State::perform_op(from_state, forward)).second;
                result.push_back(rule);
                if (forward._ops[from_state._opid]._op == RoutingTable::op_t::POP) {
                    expand_back(result); // TODO: Implement wildcard pre label in PDA instead of this.
                } else {
                    assert(forward._ops[from_state._opid]._op == RoutingTable::op_t::SWAP || forward._ops[from_state._opid]._op == RoutingTable::op_t::PUSH);
                    result.back()._pre = forward._ops[from_state._opid]._op_label;
                }
            }
            return result;
        }

    private:
        // Construction (factory)
        std::pair<bool,size_t> add_state(State&& state) {
            auto res = _states.insert(state);
            if (res.first) {
                _states.get_data(res.second) = state.accepting();
            }
            return res;
        }

        void construct_initial() {
            auto add_initial = [this](const std::vector<nfa_state_t*>& next, const Interface* inf) {
                if (inf != nullptr && inf->is_virtual()) return; // don't start on a virtual interface.
                for (const auto& n : next) {
                    auto res = add_state(State(inf, n));
                    if (res.first) _initial.push_back(res.second);
                }
            };
            for (const auto& i : _path.initial()) {
                for (const auto& e : i->_edges) {
                    auto next = e.follow_epsilon();
                    if (e.wildcard(_network.all_interfaces().size())) {
                        for (const auto& inf : _network.all_interfaces()) {
                            add_initial(next, inf->match());
                        }
                    } else if (!e._negated) {
                        for (const auto& s : e._symbols) {
                            auto inf = _network.all_interfaces()[s];
                            add_initial(next, inf->match());
                        }
                    } else {
                        for (const auto& inf : _network.all_interfaces()) {
                            if (e.contains(inf->global_id())) {
                                add_initial(next, inf->match());
                            }
                        }
                    }
                }
            }
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

        size_t set_approximation(const State& state, const RoutingTable::forward_t& forward) {
            auto num_fail = _query.number_of_failures();
            auto err = std::numeric_limits<size_t>::max();
            switch (_query.approximation()) {
                case Query::OVER:
                    return (forward._priority > num_fail) ? err : 0;
                case Query::UNDER: {
                    auto nm = state._appmode + forward._priority;
                    return (nm > num_fail) ? err : nm;
                }
                case Query::EXACT:
                case Query::DUAL:
                default:
                    return err;
            }
        }

        static std::pair<pdaaal::op_t,label_t> first_action(const RoutingTable::forward_t& forward) {
            if (forward._ops.empty()) {
                return std::make_pair(pdaaal::NOOP, label_t());
            } else {
                return convert_action(forward._ops[0]);
            }
        }
        static std::pair<pdaaal::op_t,label_t> convert_action(const RoutingTable::action_t& action) {
            pdaaal::op_t op;
            label_t op_label;
            switch (action._op) {
                case RoutingTable::op_t::POP:
                    op = pdaaal::POP;
                    break;
                case RoutingTable::op_t::PUSH:
                    op = pdaaal::PUSH;
                    op_label = action._op_label;
                    break;
                case RoutingTable::op_t::SWAP:
                    op = pdaaal::SWAP;
                    op_label = action._op_label;
                    break;
            }
            return std::make_pair(op, op_label);
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

        void add_link_to_trace(json& trace, const State& state, const std::vector<label_t>& final_header) const {
            trace.emplace_back();
            trace.back()["from_router"] = state._inf->target()->name();
            trace.back()["from_interface"] = state._inf->match()->get_name();
            trace.back()["to_router"] = state._inf->source()->name();
            trace.back()["to_interface"] = state._inf->get_name();
            trace.back()["stack"] = json::array();
            std::for_each(final_header.rbegin(), final_header.rend(), [&stack=trace.back()["stack"]](const auto& label){
                if (label == Query::bottom_of_stack()) return;
                std::stringstream s;
                s << label;
                stack.emplace_back(s.str());
            });
        }
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

        bool concreterize_trace(const std::vector<PDA::tracestate_t>& trace, std::vector<const RoutingTable::entry_t*>& entries, std::vector<const RoutingTable::forward_t*>& rules) {
            std::unordered_set<const Interface *> disabled, active;

            for (size_t sno = 0; sno < trace.size(); ++sno) {
                auto &step = trace[sno];
                State s;
                _states.unpack(step._pdastate, &s);
                if (s.ops_done()) {
                    if (sno != trace.size() - 1 && !step._stack.empty()) {
                        // peek at next element, we want to write the ops here
                        State next;
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
                State s;
                _states.unpack(step._pdastate, &s);
                if (s.ops_done()) {
                    add_link_to_trace(result_trace, s, step._stack);
                    if (cnt < entries.size()) {
                        add_rule_to_trace(result_trace, s._inf, *entries[cnt], *rules[cnt]);
                        ++cnt;
                    }
                }
            }
            return result_trace;
        }

        Network& _network;
        Query& _query;
        NFA& _path;
        std::vector<size_t> _initial;
        ptrie::map<State,bool> _states;
        const W_FN& _weight_f;
    };

    template<typename W_FN>
    NetworkPDAFactory(Query& query, Network& network, Builder::labelset_t&& all_labels, const W_FN& weight_f) -> NetworkPDAFactory<W_FN, typename W_FN::result_type>;

}

#endif /* NETWORKPDA_H */

