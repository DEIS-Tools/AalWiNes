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
 * File:   PAutomaton.cpp
 * Author: Morten K. Schou <morten@h-schou.dk>
 *
 * Created on 21-01-2020.
 */


#include "PAutomaton.h"
#include <stack>
#include <unordered_set>
#include <cassert>

namespace pdaaal {

    /*
     * PreStar, PostStar *
     */
    void PAutomaton::pre_star() {
        // This is an implementation of Algorithm 1 (figure 3.3) in:
        // Schwoon, Stefan. Model-checking pushdown systems. 2002. PhD Thesis. Technische Universität München.
        // http://www.lsv.fr/Publis/PAPERS/PDF/schwoon-phd02.pdf (page 42)
        auto& pda_states = pda().states();
        const size_t n_pda_states = pda_states.size();

        std::unordered_set<temp_edge_t, temp_edge_hasher> edges;
        std::stack<temp_edge_t> trans;
        std::vector<std::vector<std::pair<size_t,uint32_t>>> rel(_states.size());

        auto insert_edge = [&edges, &trans, this](size_t from, uint32_t label, size_t to, const trace_t *trace) {
            auto res = edges.emplace(from, label, to);
            if (res.second) { // New edge is not already in edges (rel U trans).
                trans.emplace(from, label, to);
                if (trace != nullptr) { // Don't add existing edges
                    this->add_edge(from, to, label, trace);
                }
            }
        };
        const size_t n_pda_labels = this->number_of_labels();
        auto insert_edge_bulk = [&insert_edge, n_pda_labels](size_t from, const PDA::pre_t &precondition, size_t to, const trace_t *trace) {
            if (precondition.wildcard()) { // TODO: Symbolic representation of wildcard in edges might be more efficient.
                for (uint32_t i = 0; i < n_pda_labels; i++) {
                    insert_edge(from, i, to, trace);
                }
            } else {
                for (auto &label : precondition.labels()) {
                    insert_edge(from, label, to, trace);
                }
            }
        };

        // trans := ->_0  (line 1)
        for (auto &from : this->states()) {
            for (auto &edge : from->_edges) {
                for (auto &label : edge._labels) {
                    insert_edge(from->_id, label._label, edge._to->_id, nullptr);
                }
            }
        }

        // for all <p, y> --> <p', epsilon> : trans U= (p, y, p') (line 2)
        for (size_t state = 0; state < n_pda_states; ++state) {
            const auto &rules = pda_states[state]._rules;
            for (size_t rule_id = 0; rule_id < rules.size(); ++rule_id) {
                auto &rule = rules[rule_id];
                if (rule._operation == PDA::POP) {
                    insert_edge_bulk(state, rule._precondition, rule._to, this->new_pre_trace(rule_id));
                }
            }
        }

        // delta_prime[q] = [p,rule_id]    where states[p]._rules[rule_id]._precondition.contains(y)    for each p, rule_id
        // corresponds to <p, y> --> <q, y>   (the y is the same, since we only have PUSH and not arbitrary <p, y> --> <q, y1 y2>, i.e. y==y2)
        std::vector<std::vector<std::pair<size_t, size_t>>> delta_prime(n_pda_states);

        while (!trans.empty()) { // (line 3)
            // pop t = (q, y, q') from trans (line 4)
            auto t = trans.top();
            trans.pop();
            // rel = rel U {t} (line 6)   (membership test on line 5 is done in insert_edge).
            rel[t._from].emplace_back(t._to, t._label);

            // (line 7-8 for \Delta')
            for (auto pair : delta_prime[t._from]) { // Loop over delta_prime (that match with t->from)
                auto state = pair.first;
                auto rule_id = pair.second;
                if (pda_states[state]._rules[rule_id]._precondition.contains(t._label)) {
                    insert_edge(state, t._label, t._to, this->new_pre_trace(rule_id, t._from));
                }
            }
            // Loop over \Delta (filter rules going into q) (line 7 and 9)
            if (t._from >= n_pda_states) { continue; }
            for (auto pre_state : pda_states[t._from]._pre) {
                const auto &rules = pda_states[pre_state]._rules;
                for (size_t rule_id = 0; rule_id < rules.size(); ++rule_id) {
                    auto &rule = rules[rule_id];
                    if (rule._to == t._from) { // TODO: In state._pre: also store which rule in pre_state leads to the state.
                        switch (rule._operation) {
                            case PDA::POP:
                                break;
                            case PDA::SWAP: // (line 7-8 for \Delta)
                                if (rule._op_label == t._label) {
                                    insert_edge_bulk(pre_state, rule._precondition, t._to, this->new_pre_trace(rule_id));
                                }
                                break;
                            case PDA::NOOP: // (line 7-8 for \Delta)
                                if (rule._precondition.contains(t._label)) {
                                    insert_edge(pre_state, t._label, t._to, this->new_pre_trace(rule_id));
                                }
                                break;
                            case PDA::PUSH: // (line 9)
                                if (rule._op_label == t._label) {
                                    // (line 10)
                                    delta_prime[t._to].emplace_back(pre_state, rule_id);
                                    for (auto rel_rule : rel[t._to]) { // (line 11-12)
                                        if (rule._precondition.contains(rel_rule.second)) {
                                            insert_edge(pre_state, rel_rule.second, rel_rule.first, this->new_pre_trace(rule_id, t._to));
                                        }
                                    }
                                }
                                break;
                            default:
                                assert(false);
                        }
                    }
                }
            }
        }
    }

    void PAutomaton::post_star() {
        // This is an implementation of Algorithm 2 (figure 3.4) in:
        // Schwoon, Stefan. Model-checking pushdown systems. 2002. PhD Thesis. Technische Universität München.
        // http://www.lsv.fr/Publis/PAPERS/PDF/schwoon-phd02.pdf (page 48)
        auto & pda_states = pda().states();
        auto n_pda_states = pda_states.size();

        std::unordered_set<temp_edge_t, temp_edge_hasher> edges;
        std::stack<temp_edge_t> trans;
        std::vector<temp_edge_t> rel;

        // for <p, y> -> <p', y1 y2> do  (line 3)
        //   Q' U= {q_p'y1}              (line 4)
        std::unordered_map<std::pair<size_t, uint32_t>, size_t, boost::hash<std::pair<size_t, uint32_t>>> q_prime{};
        for (auto &state : pda_states) {
            for (auto &rule : state._rules) {
                if (rule._operation == PDA::PUSH) {
                    auto res = q_prime.emplace(std::make_pair(rule._to, rule._op_label), this->next_state_id());
                    if (res.second) {
                        this->add_state(false, false);
                    }
                }
            }
        }

        std::vector<std::vector<std::pair<size_t,uint32_t>>> rel1(_states.size()); // faster access for lookup _from -> (_to, _label)

        auto insert_edge = [&edges, &trans, &rel, &rel1, this](size_t from, uint32_t label, size_t to,
                                                                const trace_t *trace,
                                                                bool direct_to_rel = false) {
            auto res = edges.emplace(from, label, to);
            if (res.second) { // New edge is not already in edges (rel U trans).
                if (direct_to_rel) {
                    rel.emplace_back(from, label, to);
                    rel1[from].emplace_back(to, label);
                } else {
                    trans.emplace(from, label, to);
                }
                if (trace != nullptr) { // Don't add existing edges
                    if (label == std::numeric_limits<uint32_t>::max()) {
                        this->add_epsilon_edge(from, to, trace);
                    } else {
                        this->add_edge(from, to, label, trace);
                    }
                }
            }
        };

        // trans := ->_0 intersect (P x Gamma x Q)  (line 1)
        // rel := ->_0 \ trans (line 2)
        for (auto &from : this->states()) {
            for (auto &edge : from->_edges) {
                assert(!edge.has_epsilon()); // PostStar algorithm assumes no epsilon transitions in the NFA.
                for (auto &label : edge._labels) {
                    insert_edge(from->_id, label._label, edge._to->_id, nullptr, from->_id >= n_pda_states);
                }
            }
        }

        while (!trans.empty()) { // (line 5)
            // pop t = (q, y, q') from trans (line 6)
            auto t = trans.top();
            trans.pop();
            // rel = rel U {t} (line 8)   (membership test on line 7 is done in insert_edge).
            rel.push_back(t);
            rel1[t._from].emplace_back(t._to, t._label);

            // if y != epsilon (line 9)
            if (t._label != std::numeric_limits<uint32_t>::max()) {
                const auto &rules = pda_states[t._from]._rules;
                for (size_t rule_id = 0; rule_id < rules.size(); ++rule_id) {
                    auto &rule = rules[rule_id];
                    if (!rule._precondition.contains(t._label)) { continue; }
                    auto trace = this->new_post_trace(t._from, rule_id, t._label);
                    switch (rule._operation) {
                        case PDA::POP: // (line 10-11)
                            insert_edge(rule._to, std::numeric_limits<uint32_t>::max(), t._to, trace);
                            break;
                        case PDA::SWAP: // (line 12-13)
                            insert_edge(rule._to, rule._op_label, t._to, trace);
                            break;
                        case PDA::NOOP:
                            insert_edge(rule._to, t._label, t._to, trace);
                            break;
                        case PDA::PUSH: // (line 14)
                            size_t q_new = q_prime[std::make_pair(rule._to, rule._op_label)];
                            insert_edge(rule._to, rule._op_label, q_new, trace); // (line 15)
                            insert_edge(q_new, t._label, t._to, trace, true); // (line 16)
                            for (auto e : rel) { // (line 17)  // TODO: Faster access to rel?
                                if (e._to == q_new && e._label == std::numeric_limits<uint32_t>::max()) {
                                    insert_edge(e._from, t._label, t._to, this->new_post_trace(q_new)); // (line 18)
                                }
                            }
                            break;
                    }
                }
            } else {
                for (auto e : rel1[t._to]) { // (line 20)
                    insert_edge(t._from, e.second, e.first, this->new_post_trace(t._to)); // (line 21)
                }
            }
        }
    }

    /*
     * Queries *
     */
    bool PAutomaton::accepts(size_t state, const std::vector<uint32_t> &stack) const {
        //Equivalent to (but hopefully faster than): return !_accept_path(state, stack).empty();

        if (stack.empty()) {
            return _states[state]->_accepting;
        }
        // DFS search.
        std::stack<std::pair<size_t, size_t>> search_stack;
        search_stack.emplace(state, 0);
        while (!search_stack.empty()) {
            auto current = search_stack.top();
            search_stack.pop();
            auto current_state = current.first;
            auto stack_index = current.second;
            for (auto &edge : _states[current_state]->_edges) {
                if (edge.contains(stack[stack_index])) {
                    auto to = edge._to->_id;
                    if (stack_index + 1 < stack.size()) {
                        search_stack.emplace(to, stack_index + 1);
                    } else if (edge._to->_accepting) {
                        return true;
                    }
                }
            }
        }
        return false;
    }

    std::vector<size_t> PAutomaton::accept_path(size_t state, const std::vector<uint32_t> &stack) const {
        if (stack.empty()) {
            if (_states[state]->_accepting) {
                return std::vector<size_t>{state};
            } else {
                return std::vector<size_t>();
            }
        }
        // DFS search.
        std::vector<size_t> path(stack.size() + 1);
        std::stack<std::pair<size_t, size_t>> search_stack;
        search_stack.emplace(state, 0);
        while (!search_stack.empty()) {
            auto current = search_stack.top();
            search_stack.pop();
            auto current_state = current.first;
            auto stack_index = current.second;
            path[stack_index] = current_state;
            for (auto &edge : _states[current_state]->_edges) {
                if (edge.contains(stack[stack_index])) {
                    auto to = edge._to->_id;
                    if (stack_index + 1 < stack.size()) {
                        search_stack.emplace(to, stack_index + 1);
                    } else if (edge._to->_accepting) {
                        path[stack_index + 1] = to;
                        return path;
                    }
                }
            }
        }
        return std::vector<size_t>();
    }


    /*
     * Modification *
     */
    void PAutomaton::edge_t::add_label(uint32_t label, const trace_t *trace) {
        label_with_trace_t label_trace{label, trace};
        auto lb = std::lower_bound(_labels.begin(), _labels.end(), label_trace);
        if (lb == std::end(_labels) || *lb != label_trace) {
            _labels.insert(lb, label_trace);
        }
    }

    bool PAutomaton::edge_t::contains(uint32_t label) {
        label_with_trace_t label_trace{label};
        auto lb = std::lower_bound(_labels.begin(), _labels.end(), label_trace);
        return lb != std::end(_labels) && *lb == label_trace;
    }

    size_t PAutomaton::add_state(bool initial, bool accepting) {
        auto id = next_state_id();
        _states.emplace_back(std::make_unique<state_t>(accepting, id));
        if (accepting) {
            _accepting.push_back(_states.back().get());
        }
        if (initial) {
            _initial.push_back(_states.back().get());
        }
        return id;
    }
    size_t PAutomaton::next_state_id() const {
        return _states.size();
    }

    void PAutomaton::add_epsilon_edge(size_t from, size_t to, const trace_t *trace) {
        auto &edges = _states[from]->_edges;
        for (auto &e : edges) {
            if (e._to->_id == to) {
                if (!e._labels.back().is_epsilon()) {
                    e._labels.emplace_back(trace);
                }
                return;
            }
        }
        edges.emplace_back(_states[to].get(), trace);
    }

    void PAutomaton::add_edge(size_t from, size_t to, uint32_t label, const trace_t *trace) {
        assert(label < std::numeric_limits<uint32_t>::max() - 1);
        auto &edges = _states[from]->_edges;
        for (auto &e : edges) {
            if (e._to->_id == to) {
                e.add_label(label, trace);
                return;
            }
        }
        edges.emplace_back(_states[to].get(), label, trace);
    }

    void PAutomaton::add_wildcard(size_t from, size_t to) {
        auto &edges = _states[from]->_edges;
        for (auto &e : edges) {
            if (e._to->_id == to) {
                e._labels.clear();
                for (uint32_t i = 0; i < number_of_labels(); i++) {
                    e._labels.emplace_back(i);
                }
                return;
            }
        }
        edges.emplace_back(_states[to].get(), number_of_labels());
    }

    /*
     * Traces *
     */
    const trace_t *PAutomaton::get_trace_label(const std::tuple<size_t, uint32_t, size_t> &edge) const {
        return get_trace_label(std::get<0>(edge), std::get<1>(edge), std::get<2>(edge));
    }
    const trace_t *PAutomaton::get_trace_label(size_t from, uint32_t label, size_t to) const {
        for (auto &e : _states[from]->_edges) {
            if (e._to->_id == to) {
                label_with_trace_t label_trace{label};
                auto lb = std::lower_bound(e._labels.begin(), e._labels.end(), label_trace);
                assert(lb != std::end(e._labels)); // We assume the edge exists.
                return lb->_trace;
            }
        }
        assert(false); // We assume the edge exists.
        return nullptr;
    }

    const trace_t *PAutomaton::new_pre_trace(size_t rule_id) {
        _trace_info.emplace_back(std::make_unique<trace_t>(rule_id, std::numeric_limits<size_t>::max()));
        return _trace_info.back().get();
    }
    const trace_t *PAutomaton::new_pre_trace(size_t rule_id, size_t temp_state) {
        _trace_info.emplace_back(std::make_unique<trace_t>(rule_id, temp_state));
        return _trace_info.back().get();
    }
    const trace_t *PAutomaton::new_post_trace(size_t from, size_t rule_id, uint32_t label) {
        _trace_info.emplace_back(std::make_unique<trace_t>(from, rule_id, label));
        return _trace_info.back().get();
    }
    const trace_t *PAutomaton::new_post_trace(size_t epsilon_state) {
        _trace_info.emplace_back(std::make_unique<trace_t>(epsilon_state));
        return _trace_info.back().get();
    }

    /*
     * Printing *
     */
    void PAutomaton::to_dot(std::ostream &out,
                                   const std::function<void(std::ostream &, const label_with_trace_t &)> &printer) const {
        out << "digraph NFA {\n";
        for (auto &s : _states) {
            out << "\"" << s->_id << "\" [shape=";
            if (s->_accepting)
                out << "double";
            out << "circle];\n";
            for (const edge_t &e : s->_edges) {
                out << "\"" << s->_id << "\" -> \"" << e._to->_id << "\" [ label=\"";
                if (e.has_non_epsilon()) {
                    out << "\\[";
                    bool first = true;
                    for (auto &l : e._labels) {
                        if (l.is_epsilon()) { continue; }
                        if (!first)
                            out << ", ";
                        first = false;
                        printer(out, l);
                    }
                    out << "\\]";
                }
                if (e._labels.size() == number_of_labels())
                    out << "*";
                if (e.has_epsilon()) {
                    if (!e._labels.empty()) out << " ";
                    out << u8"𝜀";
                }

                out << "\"];\n";
            }
        }
        for (auto &i : _initial) {
            out << "\"I" << i->_id << "\" -> \"" << i->_id << "\";\n";
            out << "\"I" << i->_id << "\" [style=invisible];\n";
        }

        out << "}\n";
    }

}
