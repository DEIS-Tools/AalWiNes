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
#include <pdaaal/PDAFactory.h>


namespace aalwines {

    template<typename W_FN = std::function<void(void)>, typename W = void>
    class NetworkPDAFactory : public pdaaal::PDAFactory<Query::label_t, W> {
        using label_t = Query::label_t;
        using NFA = pdaaal::NFA<label_t>;
        using weight_type = typename W_FN::result_type;
        static constexpr bool is_weighted = pdaaal::is_weighted<weight_type>;
        using PDAFactory = pdaaal::PDAFactory<label_t, weight_type>;
        using PDA = pdaaal::TypedPDA<label_t>;
        using rule_t = typename pdaaal::PDAFactory<label_t, weight_type>::rule_t;
    private:
        struct nstate_t {
            int32_t _appmode = 0; // mode of approximation
            int32_t _opid = -1; // which operation is the first in the rule (-1=cleaned up).
            int32_t _eid = 0; // which entry we are going for
            int32_t _rid = 0; // which rule in that entry
            NFA::state_t *_nfastate = nullptr;
            const Interface *_inf = nullptr;
        } __attribute__((packed)); // packed is needed to make this work fast with ptries
    public:
        NetworkPDAFactory(Query &query, Network &network, Builder::labelset_t&& all_labels)
        : NetworkPDAFactory(query, network, std::move(all_labels), [](){}) {};

        NetworkPDAFactory(Query &query, Network &network, Builder::labelset_t&& all_labels, const W_FN& weight_f)
        :PDAFactory(query.construction(), query.destruction(), std::move(all_labels), Query::unused_label()), _network(network),
        _query(query), _path(query.path()), _weight_f(weight_f){
            NFA::state_t *ns = nullptr;
            Interface *nr = nullptr;
            add_state(ns, nr);
            add_state(ns, nr, -1); // Add a second (different) NULL state.
            _path.compile();
            construct_initial();
        };

        [[nodiscard]] std::function<void(std::ostream &, const Query::label_t &)> label_writer() const;

        bool write_json_trace(std::ostream &stream, std::vector<PDA::tracestate_t> &trace);


    protected:
        const std::vector<size_t> &initial() override;

        [[nodiscard]] bool empty_accept() const override;

        bool accepting(size_t) override;

        std::vector<rule_t> rules(size_t) override;

        void expand_back(std::vector<rule_t> &rules);

        bool
        start_rule(size_t id, nstate_t &s, const RoutingTable::forward_t &forward, const RoutingTable::entry_t &entry,
                   NFA::state_t *destination, std::vector<rule_t> &result);

    private:
        bool
        add_interfaces(std::unordered_set<const Interface *> &disabled, std::unordered_set<const Interface *> &active,
                       const RoutingTable::entry_t &entry, const RoutingTable::forward_t &fwd) const;

        void print_trace_rule(std::ostream &stream, const Interface *inf, const RoutingTable::entry_t &entry,
                              const RoutingTable::forward_t &rule) const;

        void construct_initial();

        std::pair<bool, size_t>
        add_state(NFA::state_t *state, const Interface *inf, int32_t mode = 0, int32_t eid = 0, int32_t fid = 0, int32_t op = -1);

        int32_t set_approximation(const nstate_t &state, const RoutingTable::forward_t &forward);

        bool concreterize_trace(std::ostream &stream, const std::vector<PDA::tracestate_t> &trace,
                                std::vector<const RoutingTable::entry_t *> &entries,
                                std::vector<const RoutingTable::forward_t *> &rules);

        void write_concrete_trace(std::ostream &stream, const std::vector<PDA::tracestate_t> &trace,
                                  std::vector<const RoutingTable::entry_t *> &entries,
                                  std::vector<const RoutingTable::forward_t *> &rules);

        //void substitute_wildcards(std::vector<PDA::tracestate_t> &trace,
        //                          std::vector<const RoutingTable::entry_t *> &entries,
        //                          std::vector<const RoutingTable::forward_t *> &rules);

        Network &_network;
        Query &_query;
        NFA &_path;
        std::vector<size_t> _initial;
        ptrie::map<nstate_t, bool> _states;
        const W_FN &_weight_f;
    };

    template<typename W_FN>
    NetworkPDAFactory(Query &query, Network &network, Builder::labelset_t&& all_labels, const W_FN& weight_f) -> NetworkPDAFactory<W_FN, typename W_FN::result_type>;

    template<typename W_FN, typename W>
    void NetworkPDAFactory<W_FN, W>::construct_initial() {
        // there is potential for some serious pruning here!
        auto add_initial = [&, this](NFA::state_t *state, const Interface *inf) {
            if (inf != nullptr && inf->is_virtual()) // don't start on a virtual interface.
                return;
            std::vector<NFA::state_t *> next{state};
            NFA::follow_epsilon(next);
            for (auto &n : next) {
                auto res = add_state(n, inf, 0, 0, 0, -1);
                if (res.first)
                    _initial.push_back(res.second);
            }
        };

        for (auto i : _path.initial()) {
            // the path is one behind, we are coming from an unknown router via an OK interface
            // i.e. we have to move straight to the next state
            for (auto &e : i->_edges) {
                if (e.wildcard(_network.all_interfaces().size())) {
                    for (auto inf : _network.all_interfaces()) {

                        add_initial(e._destination, inf->match());
                    }
                } else if (!e._negated) {
                    for (auto s : e._symbols) {
                        auto inf = _network.all_interfaces()[s];
                        add_initial(e._destination, inf->match());
                    }
                } else {
                    for (auto inf : _network.all_interfaces()) {
                        auto iid = inf->global_id();
                        auto lb = std::lower_bound(e._symbols.begin(), e._symbols.end(), iid);
                        assert(std::is_sorted(e._symbols.begin(), e._symbols.end()));
                        if (lb == std::end(e._symbols) || *lb != iid) {
                            add_initial(e._destination, inf->match());
                        }
                    }
                }
            }
        }
        std::sort(_initial.begin(), _initial.end());
    }

    template<typename W_FN, typename W>
    std::pair<bool, size_t>
    NetworkPDAFactory<W_FN, W>::add_state(NFA::state_t *state, const Interface *inf, int32_t mode, int32_t eid, int32_t fid, int32_t op) {
        nstate_t ns;
        ns._appmode = mode;
        ns._nfastate = state;
        ns._opid = op;
        ns._inf = inf;
        ns._rid = fid;
        ns._eid = eid;
        auto res = _states.insert(ns);
        if (res.first) {
            if (res.second > 1) {
                // don't check null-state
                auto &d = _states.get_data(res.second);
                d = op == -1 && (inf == nullptr || !inf->is_virtual()) && state->_accepting;
            }
        }
        return res;
    }

    template<typename W_FN, typename W>
    const std::vector<size_t> &NetworkPDAFactory<W_FN, W>::initial() {
        return _initial;
    }

    template<typename W_FN, typename W>
    bool NetworkPDAFactory<W_FN, W>::empty_accept() const {
        for (auto s : _path.initial()) {
            if (s->_accepting)
                return true;
        }
        return false;
    }

    template<typename W_FN, typename W>
    bool NetworkPDAFactory<W_FN, W>::accepting(size_t i) {
        return _states.get_data(i);
    }

    template<typename W_FN, typename W>
    int32_t NetworkPDAFactory<W_FN, W>::set_approximation(const nstate_t &state, const RoutingTable::forward_t &forward) {
        auto num_fail = _query.number_of_failures();
        auto err = std::numeric_limits<int32_t>::max();
        switch (_query.approximation()) {
            case Query::OVER:
                if ((int) forward._priority > num_fail)
                    return err;
                else
                    return 0;
            case Query::UNDER: {
                auto nm = state._appmode + forward._priority;
                if ((int) nm > num_fail)
                    return err;
                else
                    return nm;
            }
            case Query::EXACT:
            case Query::DUAL:
            default:
                return err;
        }
    }

    template<typename W_FN, typename W>
    bool NetworkPDAFactory<W_FN, W>::start_rule(size_t id, nstate_t &s, const RoutingTable::forward_t &forward,
                                             const RoutingTable::entry_t &entry, NFA::state_t *destination,
                                             std::vector<NetworkPDAFactory::rule_t> &result) {
        rule_t nr;
        auto appmode = s._appmode;
        if (!forward._via->is_virtual()) {
            appmode = set_approximation(s, forward);
            if (appmode == std::numeric_limits<int32_t>::max())
                return false;
        }
        if (!forward._ops.empty()) {
            switch (forward._ops[0]._op) {
                case RoutingTable::op_t::POP:
                    nr._op = pdaaal::POP;
                    break;
                case RoutingTable::op_t::PUSH:
                    nr._op = pdaaal::PUSH;
                    nr._op_label = forward._ops[0]._op_label;
                    break;
                case RoutingTable::op_t::SWAP:
                    nr._op = pdaaal::SWAP;
                    nr._op_label = forward._ops[0]._op_label;
                    break;
            }
        } else {
            if (!entry.ignores_label()) {
                nr._op = pdaaal::SWAP;
                nr._op_label = entry._top_label;
            } else {
                nr._op = pdaaal::NOOP;
            }
        }

        std::vector<NFA::state_t *> next{destination};
        if (forward._via->is_virtual())
            next = {s._nfastate};
        else
            NFA::follow_epsilon(next);
        for (auto &n : next) {
            result.emplace_back(nr);
            auto &ar = result.back();
            std::pair<bool, size_t> res;
            if (forward._ops.size() <= 1) {
                res = add_state(n, forward._via->match(), appmode);
                if constexpr (is_weighted) {
                    ar._weight = _weight_f(forward, entry);
                }
            } else {
                auto eid = ((&entry) - s._inf->table().entries().data());
                auto rid = ((&forward) - entry._rules.data());
                res = add_state(n, s._inf, appmode, eid, rid, 0);
            }
            ar._dest = res.second;
            if (entry.ignores_label()) {
                expand_back(result); // TODO: Implement wildcard pre label in PDA instead of this.
            } else {
                ar._pre = entry._top_label;
            }
        }
        return true;
    }
    template<typename W_FN, typename W>
    void NetworkPDAFactory<W_FN, W>::expand_back(std::vector<rule_t>& rules) {
        rule_t cpy;
        std::swap(rules.back(), cpy);
        rules.pop_back();
        for (const auto& label : this->_all_labels) {
            rules.push_back(cpy);
            rules.back()._pre = label;
        }
    }

    template<typename W_FN, typename W>
    std::vector<typename NetworkPDAFactory<W_FN, W>::rule_t> NetworkPDAFactory<W_FN, W>::rules(size_t id) {
        if (_query.approximation() == Query::EXACT ||
            _query.approximation() == Query::DUAL) {
            throw base_error("Exact and Dual analysis method not yet supported");
        }
        nstate_t s;
        _states.unpack(id, &s);
        std::vector<typename NetworkPDAFactory<W_FN, W>::rule_t> result;
        if (s._opid < 0) {
            if (s._inf == nullptr)
                return result;
            // all clean! start pushing.
            for (auto &entry : s._inf->table().entries()) {
                for (auto &forward : entry._rules) {
                    assert(forward._via != nullptr && forward._via->target() != nullptr);
                    if (forward._via->is_virtual()) {
                        if (!start_rule(id, s, forward, entry, s._nfastate, result))
                            continue;
                    } else {
                        for (auto &e : s._nfastate->_edges) {
                            if (e.empty(_network.all_interfaces().size())) {
                                continue;
                            }
                            auto lb = std::lower_bound(e._symbols.begin(), e._symbols.end(), forward._via->global_id());
                            bool found = lb != std::end(e._symbols) && *lb == forward._via->global_id();

                            if (found != (!e._negated)) {
                                continue;
                            } else {
                                if (!start_rule(id, s, forward, entry, e._destination, result))
                                    continue;
                            }
                        }
                    }
                }
            }
        } else {
            auto &r = s._inf->table().entries()[s._eid]._rules[s._rid];
            result.emplace_back();
            auto &nr = result.back();
            auto &act = r._ops[s._opid + 1];
            nr._pre = s._inf->table().entries()[s._eid]._top_label;
            for (auto pid = 0; pid <= s._opid; ++pid) {
                switch (r._ops[pid]._op) {
                    case RoutingTable::op_t::SWAP:
                    case RoutingTable::op_t::PUSH:
                        nr._pre = r._ops[pid]._op_label;
                        break;
                    case RoutingTable::op_t::POP:
                    default:
                        throw base_error("Unexpected pop!");
                        assert(false);
                }
            }
            switch (act._op) {
                case RoutingTable::op_t::POP:
                    nr._op = pdaaal::POP;
                    throw base_error("Unexpected pop!");
                    assert(false);
                    break;
                case RoutingTable::op_t::PUSH:
                    nr._op = pdaaal::PUSH;
                    nr._op_label = act._op_label;
                    break;
                case RoutingTable::op_t::SWAP:
                    nr._op = pdaaal::SWAP;
                    nr._op_label = act._op_label;
                    break;
            }
            //Also handle nr  (Routerhop(Latensy), Network Stack Size, traffic engineergroup failiures) (does the implementaion approache work?) (Define MPLS in report)
            if (s._opid + 2 == (int) r._ops.size()) {
                auto res = add_state(s._nfastate, r._via->match(), s._appmode);
                nr._dest = res.second;
                if constexpr (is_weighted) {
                    nr._weight = _weight_f(r, s._inf->table().entries()[s._eid]);
                }
            } else {
                auto res = add_state(s._nfastate, s._inf, s._appmode, s._eid, s._rid, s._opid + 1);
                nr._dest = res.second;
            }
            //or/andHere
        }
        return result;
    }

    template<typename W_FN, typename W>
    void NetworkPDAFactory<W_FN, W>::print_trace_rule(std::ostream &stream, const Interface* inf,
                                                  const RoutingTable::entry_t &entry,
                                                  const RoutingTable::forward_t &rule) const {
        stream << "{";

        auto name = inf->source()->interface_name(inf->id());
        stream << R"("ingoing":")" << name << "\"";
        stream << ",\"pre\":";
        if (entry.ignores_label()) {
            stream  << "\"null\"";
        } else {
            stream << "\"" << entry._top_label << "\"";
        }
        stream << ",\"rule\":";
        rule.print_json(stream, false);

        if constexpr (is_weighted) {
            stream << ", \"priority-weight\": [";
            auto weights = _weight_f(rule, entry);
            for (size_t v = 0; v < weights.size(); v++){
                if (v != 0) stream << ", ";
                stream << "\"" << std::to_string(weights[v]) << "\"";
            }
            stream << "]";
        }
        stream << "}";
    }

    template<typename W_FN, typename W>
    bool NetworkPDAFactory<W_FN, W>::add_interfaces(std::unordered_set<const Interface *> &disabled,
                                                 std::unordered_set<const Interface *> &active,
                                                 const RoutingTable::entry_t &entry,
                                                 const RoutingTable::forward_t &fwd) const {
        auto *inf = fwd._via;
        if (disabled.count(inf) > 0) {
            return false; // should be down!
        }
        // find all rules with a "covered" pre but lower weight.
        // these must have been disabled!
        if (fwd._priority == 0)
            return true;
        std::unordered_set<const Interface *> tmp = disabled;

        // find failing rule and disable _via interface
        for(auto &rule : entry._rules){
            if (rule._priority < fwd._priority) {
                if (active.count(rule._via) > 0) return false;
                tmp.insert(rule._via);
                if (tmp.size() > (uint32_t) _query.number_of_failures()) {
                    return false;
                }
                break;
            }
        }
        if (tmp.size() > (uint32_t) _query.number_of_failures()) {
            return false;
        } else {
            disabled.swap(tmp);
            active.insert(fwd._via);
            return true;
        }
    }

    template<typename W_FN, typename W>
    bool NetworkPDAFactory<W_FN, W>::concreterize_trace(std::ostream &stream, const std::vector<PDA::tracestate_t> &trace,
                                                     std::vector<const RoutingTable::entry_t *> &entries,
                                                     std::vector<const RoutingTable::forward_t *> &rules) {
        std::unordered_set<const Interface *> disabled, active;

        for (size_t sno = 0; sno < trace.size(); ++sno) {
            auto &step = trace[sno];
            if (step._pdastate > 1 && step._pdastate < this->_num_pda_states) {
                // handle, lookup right states
                nstate_t s;
                _states.unpack(step._pdastate, &s);
                if (s._opid >= 0) {
                    // Skip, we are just doing a bunch of ops here, printed elsewhere.
                } else {
                    if (sno != trace.size() - 1 && trace[sno + 1]._pdastate > 1 &&
                        trace[sno + 1]._pdastate < this->_num_pda_states && !step._stack.empty()) {
                        // peek at next element, we want to write the ops here
                        nstate_t next;
                        _states.unpack(trace[sno + 1]._pdastate, &next);
                        if (next._opid != -1) {
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
                                            ok = ((ssize_t) r._priority) == (next._appmode - s._appmode);
                                            break;
                                        case Query::OVER:
                                            ok = ((ssize_t) r._priority) <= _query.number_of_failures();
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
                                stream << "{\"pre\":\"error\"}";
                                return false;
                            }
                        }
                    }
                }
            } else {
                // construction, destruction, BORRING!
                // SKIP
            }
        }
        return true;
    }

    template<typename W_FN, typename W>
    void
    NetworkPDAFactory<W_FN, W>::write_concrete_trace(std::ostream &stream, const std::vector<PDA::tracestate_t> &trace,
                                                  std::vector<const RoutingTable::entry_t *> &entries,
                                                  std::vector<const RoutingTable::forward_t *> &rules) {
        bool first = true;
        size_t cnt = 0;
        for (const auto &step : trace) {
            if (step._pdastate > 1 && step._pdastate < this->_num_pda_states) {
                nstate_t s;
                _states.unpack(step._pdastate, &s);
                if (s._opid >= 0) {
                    // Skip, we are just doing a bunch of ops here, printed elsewhere.
                } else {
                    if (!first)
                        stream << ",\n";
                    stream << "\t\t\t{";
                    assert(s._inf != nullptr);
                    auto from_inf = s._inf->match();
                    auto from_router = s._inf->target();
                    auto to_inf = s._inf;
                    auto to_router = s._inf->source();
                    assert(from_inf != nullptr);
                    assert(from_router != nullptr);
                    assert(to_router != nullptr);
                    stream << R"("from_router":")" << from_router->name() << "\""
                           << R"(,"from_interface":")" << from_router->interface_name(from_inf->id()) << "\""
                           << R"(,"to_router":")" << to_router->name() << "\""
                           << R"(,"to_interface":")" << to_router->interface_name(to_inf->id()) << "\"";
                    stream << ",\"stack\":[";
                    bool first_symbol = true;
                    for (auto &symbol : step._stack) {
                        if (symbol == Query::bottom_of_stack()) continue;
                        if (!first_symbol) stream << ",";
                        stream << "\"" << symbol;
                        //if (_network.is_service_label(symbol))
                        //    stream << "^";
                        stream << "\"";
                        first_symbol = false;
                    }
                    stream << "]}";
                    if (cnt < entries.size()) {
                        stream << ",\n\t\t\t";
                        print_trace_rule(stream, s._inf, *entries[cnt], *rules[cnt]);
                        ++cnt;
                    }
                    first = false;
                }
            }
        }
    }

    template<typename W_FN, typename W>
    bool NetworkPDAFactory<W_FN, W>::write_json_trace(std::ostream &stream, std::vector<PDA::tracestate_t> &trace) {

        std::vector<const RoutingTable::entry_t *> entries;
        std::vector<const RoutingTable::forward_t *> rules;

        if (!concreterize_trace(stream, trace, entries, rules)) {
            return false;
        }

        // Do the printing
        write_concrete_trace(stream, trace, entries, rules);
        return true;
    }

    template<typename W_FN, typename W>
    std::function<void(std::ostream &, const Query::label_t &)> NetworkPDAFactory<W_FN, W>::label_writer() const {
        return [](std::ostream &s, const Query::label_t &label) {
            RoutingTable::entry_t::print_label(label, s, false);
        };
    }


}

#endif /* NETWORKPDA_H */

