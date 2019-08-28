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
 * File:   PDA.cpp
 * Author: Peter G. Jensen <root@petergjoel.dk>
 * 
 * Created on August 21, 2019, 2:47 PM
 */

#include "PDA.h"

#include "utils/errors.h"

#include <queue>
#include <cassert>

namespace pdaaal
{

    PDA::PDA()
    {

    }

    PDA::~PDA()
    {

    }

    size_t PDA::size() const
    {
        size_t cnt = 1;
        // lets start by the initial transitions
        auto& is = states()[0];
        cnt += is._rules.size();
        for (size_t sid = 1; sid < states().size(); ++sid) {
            const state_t& state = states()[sid];
            for (auto& r : state._rules) {
                if (r._to == 0) {
                    ++cnt;
                    continue;
                }
                if (r._precondition.empty()) continue;
                cnt += r._precondition.wildcard() ? number_of_labels() : r._precondition.labels().size();
            }
        }
        return cnt;
    }

    const std::vector<PDA::state_t>& PDA::states() const
    {
        return _states;
    }

    void PDA::_add_rule(size_t from, size_t to, op_t op, uint32_t label, bool negated, const std::vector<uint32_t>& pre)
    {
        auto mm = std::max(from, to);
        if (mm >= _states.size())
            _states.resize(mm + 1);
        rule_t r;
        r._to = to;
        r._op_label = label;
        r._operation = op;
        auto& rules = _states[from]._rules;
        auto lb = std::lower_bound(rules.begin(), rules.end(), r);
        if (lb == std::end(rules) || *lb != r)
            lb = rules.insert(lb, r); // TODO this is expensive. Use lists?

        lb->_precondition.merge(negated, pre, number_of_labels());
        auto& prestate = _states[to]._pre;
        auto lpre = std::lower_bound(prestate.begin(), prestate.end(), from);
        if (lpre == std::end(prestate) || *lpre != from)
            prestate.insert(lpre, from);
    }

    void PDA::backwards_prune()
    {
        std::queue<size_t> waiting;
        std::vector<bool> seen(_states.size());
        waiting.push(0);
        seen[0] = true;
        while (!waiting.empty()) {
            // forward
            auto el = waiting.front();
            waiting.pop();
            for (auto& s2 : _states[el]._pre) {
                if (!seen[s2]) {
                    waiting.push(s2);
                    seen[s2] = true;
                }
            }
        }
        for (size_t s = 0; s < _states.size(); ++s) {
            if (!seen[s]) {
                clear_state(s);
            }
        }
    }

    void PDA::forwards_prune()
    {
        // lets do a forward/backward pruning first
        std::queue<size_t> waiting;
        std::vector<bool> seen(_states.size());
        waiting.push(0);
        seen[0] = true;
        while (!waiting.empty()) {
            // forward
            auto el = waiting.front();
            waiting.pop();
            for (auto& r : _states[el]._rules) {
                if (!seen[r._to] && !r._precondition.empty()) {
                    waiting.push(r._to);
                    seen[r._to] = true;
                }
            }
        }
        for (size_t s = 0; s < _states.size(); ++s) {
            if (!seen[s]) {
                clear_state(s);
            }
        }
    }

    void PDA::target_tos_prune()
    {
        std::queue<size_t> waiting;
        std::vector<bool> in_waiting(_states.size());
        in_waiting[0] = true; // we don't ever prune 0.
        for (size_t t = 0; t < _states.size(); ++t) {
            in_waiting[t] = true;
            waiting.push(t);
        }

        while (!waiting.empty()) {
            auto s = waiting.front();
            waiting.pop();
            if (s == 0) continue;
            in_waiting[s] = false;
            std::set<uint32_t> usefull_tos;
            bool cont = false;
            for (auto& r : _states[s]._rules) {
                if (!r._precondition.wildcard()) {
                    usefull_tos.insert(r._precondition.labels().begin(), r._precondition.labels().end());
                }
                else {
                    cont = true;
                }
                if (usefull_tos.size() == number_of_labels() || cont) {
                    cont = true;
                    break;
                }
            }
            if (cont)
                continue;
            for (auto& pres : _states[s]._pre) {
                auto& state = _states[pres];
                for (auto& r : state._rules) {
                    if (r._to == s) {
                        switch (r._operation) {
                        case SWAP:
                        case PUSH:
                            if (!r._precondition.empty() && usefull_tos.count(r._op_label) == 0) {
                                r._precondition.clear();
                                if (!in_waiting[pres])
                                    waiting.push(pres);
                                in_waiting[pres] = true;
                            }
                            break;
                        case NOOP:
                        {
                            bool changed = r._precondition.noop_pre_filter(usefull_tos);
                            if (changed && !in_waiting[pres]) {
                                waiting.push(pres);
                                in_waiting[pres] = true;
                            }
                            break;
                        }
                        case POP:
                            // we cant really prune this one.
                            // it fans out if we try; i.e. no local computation.
                            break;
                        }
                    }
                }
            }
        }
    }

    void PDA::clear_state(size_t s)
    {
        _states[s]._rules.clear();
        for (auto& p : _states[s]._pre) {
            auto rit = _states[p]._rules.begin();
            auto wit = rit;
            for (; rit != std::end(_states[p]._rules); ++rit) {
                if (rit->_to != s) {
                    if (wit != rit)
                        *wit = *rit;
                    ++wit;
                }
            }
            _states[p]._rules.resize(wit - std::begin(_states[p]._rules));
        }
        _states[s]._pre.clear();
    }

    std::pair<size_t, size_t> PDA::reduce(int aggresivity)
    {
        size_t cnt = size();
        if (aggresivity == 0)
            forwards_prune();
        backwards_prune();
        std::queue<size_t> waiting;
        if (aggresivity == 0) {
            size_t after_cnt = size();
            return std::make_pair(cnt, after_cnt);
        }
        auto ds = (aggresivity >= 2);
        std::vector<tos_t> approximation(_states.size());
        // initialize
        for (auto& r : _states[0]._rules) {
            if (r._to == 0) continue;
            if (r._precondition.empty()) continue;
            auto& ss = approximation[r._to];
            std::pair<bool, bool> tmp(true, true);
            if (ss._in_waiting == tos_t::NOT_IN_STACK) {
                ss.update_state(tmp);
                waiting.push(r._to);
            }
            assert(r._operation == PUSH);
            auto lb = std::lower_bound(ss._tos.begin(), ss._tos.end(), r._op_label);
            if (lb == std::end(ss._tos) || *lb != r._op_label)
                ss._tos.insert(lb, r._op_label);
        }

        if (aggresivity == 3) {
            target_tos_prune();
        }
        // saturate
        while (!waiting.empty()) {
            auto el = waiting.front();
            waiting.pop();
            auto& ss = approximation[el];
            auto& state = _states[el];
            auto fit = state._rules.begin();
            ss._in_waiting = tos_t::NOT_IN_STACK;
            while (fit != std::end(state._rules)) {
                if (fit->_to == 0) {
                    ++fit;
                    continue;
                }
                if (fit->_precondition.empty()) {
                    ++fit;
                    continue;
                }
                auto& to = approximation[fit->_to];
                // handle dots!
                std::pair<bool, bool> change;
                switch (fit->_operation) {
                case POP:
                    change = to.merge_pop(ss, *fit, ds, number_of_labels());
                    break;
                case NOOP:
                    change = to.merge_noop(ss, *fit, ds, number_of_labels());
                    break;
                case PUSH:
                    change = to.merge_push(ss, *fit, ds, number_of_labels());
                    break;
                case SWAP:
                    change = to.merge_swap(ss, *fit, ds, number_of_labels());
                    break;
                default:
                    throw base_error("Unknown PDA operation");
                    break;
                }
                if (to.update_state(change)) {
                    waiting.push(fit->_to);
                }
                ++fit;
            }
        }
        // DO PRUNING!
        for (size_t i = 1; i < _states.size(); ++i) {
            auto& app = approximation[i];
            auto& state = _states[i];
            size_t br = 0;
            for (size_t r = 0; r < state._rules.size(); ++r) {
                // check rule
                auto& rule = state._rules[r];
                if (rule._to == 0 || rule._precondition.intersect(app, number_of_labels())) {
                    if (br != r) {
                        std::swap(state._rules[br], state._rules[r]);
                    }
                    ++br;
                }
            }
            state._rules.resize(br);
            if (state._rules.empty()) {
                clear_state(i);
            }
        }

        backwards_prune();
        if (aggresivity == 3) {
            // it could potentially work as fixpoint; not sure if it has any effect.
            target_tos_prune();
        }

        size_t after_cnt = size();
        return std::make_pair(cnt, after_cnt);
    }

    void PDA::pre_t::merge(bool negated, const std::vector<uint32_t>& other, size_t all_labels)
    {
        if (_wildcard) return;
        if (negated && other.empty()) {
            _wildcard = true;
            _labels.clear();
            return;
        }

        if (!negated) {
            auto lid = _labels.begin();
            for (auto fit = other.begin(); fit != other.end(); ++fit) {
                while (lid != _labels.end() && *lid < *fit) ++lid;
                if (lid == _labels.end()) {
                    _labels.insert(_labels.end(), fit, other.end());
                    break;
                }
                if (*lid == *fit) continue;
                lid = _labels.insert(lid, *fit);
                ++lid;
            }
        }
        else {
            auto lid = _labels.begin();
            uint32_t ait = 0;
            for (auto fit = other.begin(); fit != other.end(); ++fit) {
                uint32_t nm = ait;
                while (nm != all_labels && nm < *fit) ++nm;
                // assert(*nm == *fit); TODO, check if this is ok
                for (; ait < nm; ++ait) {
                    assert(ait == nm);
                    while (lid != std::end(_labels) && *lid < ait) ++lid;
                    if (lid == std::end(_labels)) {
                        for (auto n = ait; n < nm; ++n)
                            _labels.insert(_labels.end(), n);
                        lid = std::end(_labels);
                        break;
                    }
                    else if (ait == *lid) {
                        continue;
                    }
                    else {
                        assert(ait < *lid);
                        lid = _labels.insert(lid, ait);
                        ++lid;
                    }
                }

            }
            for (; ait < all_labels; ++ait) {
                while (lid != std::end(_labels) && *lid < ait) ++lid;
                if (lid == std::end(_labels)) {
                    for (uint32_t n = ait; n < all_labels; ++n)
                        _labels.insert(_labels.end(), n);
                    lid = std::end(_labels);
                    break;
                }
                else if (ait == *lid) continue;
                else {
                    assert(ait < *lid);
                    lid = _labels.insert(lid, ait);
                    ++lid;
                }
            }
        }

        if (_labels.size() == all_labels) {
            _wildcard = true;
            _labels.clear();
        }
        assert(std::is_sorted(_labels.begin(), _labels.end()));
    }

    bool PDA::pre_t::noop_pre_filter(const std::set<uint32_t>& usefull)
    {
        if (_wildcard) {
            _labels.insert(_labels.begin(), usefull.begin(), usefull.end());
            _wildcard = false;
            return true;
        }
        else {
            auto it = _labels.begin();
            auto wit = it;
            auto uit = usefull.begin();
            while (it != _labels.end()) {
                while (uit != std::end(usefull) && *uit < *it) ++uit;
                if (uit != std::end(usefull) && *uit == *it) {
                    *wit = *it;
                    ++wit;
                    ++it;
                }
                else {
                    ++it;
                }
            }
            if (wit != it) {
                _labels.resize(wit - _labels.begin());
                return true;
            }
        }
        return false;
    }

    bool PDA::pre_t::contains(uint32_t label) const
    {
        if(_wildcard) return true;
        auto lb = std::lower_bound(_labels.begin(), _labels.end(), label);
        if(lb == std::end(_labels) || *lb != label)
            return false;
        return true;
    }

    
    bool PDA::pre_t::intersect(const tos_t& tos, size_t all_labels)
    {
        if (tos._tos.size() == all_labels) {
            //            std::cerr << "EMPY 1" << std::endl;
            return !empty();
        }
        if (_wildcard) {
            if (tos._tos.size() == all_labels)
                return empty();
            else {
                _labels = tos._tos;
                _wildcard = false;
            }
        }
        else {
            auto fit = tos._tos.begin();
            size_t bit = 0;
            for (size_t nl = 0; nl < _labels.size(); ++nl) {
                while (fit != std::end(tos._tos) && *fit < _labels[nl]) ++fit;
                if (fit == std::end(tos._tos)) break;
                if (*fit == _labels[nl]) {
                    _labels[bit] = _labels[nl];
                    ++bit;
                }
            }
            _labels.resize(bit);
            assert(_labels.size() != all_labels);
        }
        return !empty();
    }

    const std::vector<uint32_t>& PDA::pre_t::labels() const
    {
        return _labels;
    }

    void PDA::pre_t::clear()
    {
        _wildcard = false;
        _labels.clear();
    }

    bool PDA::pre_t::empty() const
    {
        return !_wildcard && _labels.empty();
    }

    bool PDA::pre_t::wildcard() const
    {
        return _wildcard;
    }

    bool PDA::rule_t::is_terminal() const
    {
        return _to == 0;
    }

    bool PDA::rule_t::operator!=(const rule_t& other) const
    {
        return !(*this == other);
    }

    bool PDA::rule_t::operator<(const rule_t& other) const
    {
        if (_op_label != other._op_label)
            return _op_label < other._op_label;
        if (_to != other._to)
            return _to < other._to;
        return _operation < other._operation;
    }

    bool PDA::rule_t::operator==(const rule_t& other) const
    {
        return _to == other._to && _op_label == other._op_label && _operation == other._operation;
    }

    bool PDA::tos_t::active(const tos_t& prev, const rule_t& rule)
    {
        if (rule._precondition.empty())
            return false;
        else if (rule._precondition.wildcard()) {
            return true;
        }
        else {
            auto rit = rule._precondition.labels().begin();
            bool match = false;
            for (auto& s : prev._tos) {
                while (rit != std::end(rule._precondition.labels()) && *rit < s) ++rit;
                if (rit == std::end(rule._precondition.labels())) break;
                if (*rit == s) {
                    match = true;
                    break;
                }
            }
            return match;
        }
        return true;
    }

    bool PDA::tos_t::empty_tos() const
    {
        return _tos.empty();
    }

    bool PDA::tos_t::forward_stack(const tos_t& prev, size_t all_labels)
    {
        auto os = _stack.size();
        if (_stack.size() != all_labels) {
            if (prev._stack.size() == all_labels) {
                _stack = prev._stack;
            }
            else {
                auto lid = _stack.begin();
                auto pid = prev._stack.begin();
                while (pid != std::end(prev._stack)) {
                    while (lid != _stack.end() && *lid < *pid) ++lid;
                    if (lid != _stack.end() && *lid == *pid) {
                        ++pid;
                        continue;
                    }
                    if (lid == _stack.end()) break;
                    auto oid = pid;
                    while (pid != std::end(prev._stack) && *pid < *lid) ++pid;
                    lid = _stack.insert(lid, oid, pid);
                }
                _stack.insert(lid, pid, std::end(prev._stack));
            }
        }
        return os != _stack.size();
    }

    std::pair<bool, bool> PDA::tos_t::merge_noop(const tos_t& prev, const rule_t& rule, bool dual_stack, size_t all_labels)
    {
        if (rule._precondition.empty()) return std::make_pair(false, false);
        bool changed = false;
        {
            auto iit = _tos.begin();
            for (auto& symbol : rule._precondition.wildcard() ? prev._tos : rule._precondition.labels()) {
                while (iit != _tos.end() && *iit < symbol) ++iit;
                if (iit != _tos.end() && *iit == symbol) {
                    ++iit;
                    continue;
                }
                changed = true;
                iit = _tos.insert(iit, symbol);
                ++iit;
            }
        }
        bool stack_changed = false;
        if (dual_stack) {
            stack_changed |= forward_stack(prev, all_labels);
        }
        return std::make_pair(changed, stack_changed);
    }

    std::pair<bool, bool> PDA::tos_t::merge_pop(const tos_t& prev, const rule_t& rule, bool dual_stack, size_t all_labels)
    {
        if (!active(prev, rule)) {
            return std::make_pair(false, false);
        }
        bool changed = false;
        bool stack_changed = false;
        if (!dual_stack) {
            if (_tos.size() != all_labels) {
                _tos.resize(all_labels);
                for (size_t i = 0; i < all_labels; ++i) _tos[i] = i;
                changed = true;
            }
        }
        else {
            // move stack->stack
            stack_changed |= forward_stack(prev, all_labels);
            // move stack -> TOS
            if (_tos.size() != all_labels) {
                if (prev._stack.size() == all_labels) {
                    changed = true;
                    _tos = prev._stack;
                }
                else {
                    auto it = _tos.begin();
                    for (auto s : prev._stack) {
                        while (it != std::end(_tos) && *it < s) ++it;
                        if (it != std::end(_tos) && *it == s) continue;
                        it = _tos.insert(it, s);
                        changed = true;
                    }
                }
            }
        }
        return std::make_pair(changed, stack_changed);
    }

    std::pair<bool, bool> PDA::tos_t::merge_push(const tos_t& prev, const rule_t& rule, bool dual_stack, size_t all_labels)
    {
        // similar to swap!
        auto changed = merge_swap(prev, rule, dual_stack, all_labels);
        if (dual_stack) {
            // but we also push all TOS labels down
            if (_stack.size() != all_labels) {
                if (prev._tos.size() == all_labels) {
                    changed.second = true;
                    _stack = prev._tos;
                }
                else {
                    auto it = _stack.begin();
                    for (auto& s : prev._tos) {
                        while (it != _stack.end() && *it < s) ++it;
                        if (it != std::end(_stack) && *it == s) continue;
                        it = _stack.insert(it, s);
                        changed.second = true;
                    }
                }
            }
        }
        return changed;
    }

    std::pair<bool, bool> PDA::tos_t::merge_swap(const tos_t& prev, const rule_t& rule, bool dual_stack, size_t all_labels)
    {
        if (!active(prev, rule))
            return std::make_pair(false, false); // we know that there is a match!
        bool changed = false;
        {
            auto lb = std::lower_bound(_tos.begin(), _tos.end(), rule._op_label);
            if (lb == std::end(_tos) || *lb != rule._op_label) {
                changed = true;
                _tos.insert(lb, rule._op_label);
            }
        }
        bool stack_changed = false;
        if (dual_stack) {
            stack_changed |= forward_stack(prev, all_labels);
        }
        return std::make_pair(changed, stack_changed);
    }

    bool PDA::tos_t::update_state(const std::pair<bool, bool>& new_state)
    {
        auto pre = _in_waiting;
        switch (_in_waiting) {
        case TOS:
        case BOS:
        case NOT_IN_STACK:
            if (new_state.first)
                _in_waiting = (waiting_t) (_in_waiting | TOS);
            if (new_state.second)
                _in_waiting = (waiting_t) (_in_waiting | BOS);
            return (_in_waiting != pre);
            break;
        case BOTH:
            return false;
            break;
        }
        assert(false);
        return false;
    }

}
