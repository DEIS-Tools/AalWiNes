/*
 *  Copyright Peter G. Jensen, all rights reserved.
 */

/* 
 * File:   PDA.h
 * Author: Peter G. Jensen <root@petergjoel.dk>
 *
 * Created on July 23, 2019, 1:34 PM
 */

#ifndef PDA_H
#define PDA_H

#include "utils/errors.h"

#include <ptrie_map.h>

#include <vector>
#include <queue>
#include <unordered_set>


namespace pdaaal {
    template<typename T>
    class PDAFactory;
    
    template<typename T>
    class PDA {
    public:
        enum op_t {
            PUSH = 1, 
            POP = 2, 
            SWAP = 4, 
            NOOP = 8
        };
        struct tracestate_t {
            size_t _pdastate = 0;
            std::vector<std::pair<bool,T>> _stack;
        };
    public:
        struct tos_t;
        struct pre_t {
        private:
            bool _wildcard = false;
            std::vector<T> _labels;            
        public:
            bool wildcard() const { return _wildcard; }
            auto& labels() const { return _labels; }
            void merge(bool negated, const std::vector<T>& other, const std::vector<T>& all_labels);
            void clear() {
                _wildcard = false;
                _labels.clear();
            }
            bool empty() const {
                return !_wildcard && _labels.empty();
            }
            bool intersect(const tos_t& tos, const std::vector<T>& all_labels);
            void noop_pre_filter(const std::unordered_set<T>& usefull);
        };

        struct rule_t {
            size_t _to = 0;
            op_t _operation = NOOP;
            T _op_label = T{};
            bool is_terminal() const 
            {
                return _to == 0;
            }
            
            bool operator<(const rule_t& other) const
            {
                if(_op_label != other._op_label)
                    return _op_label < other._op_label;
                if(_to != _to)
                    return _to < other._to;
                return _operation < other._operation;
            }
            
            bool operator==(const rule_t& other) const
            {
                return _to == other._to && _op_label == other._op_label && _operation == other._operation;
            }
            
            bool operator!=(const rule_t& other) const {
                return !(*this == other);
            }
            
            pre_t _precondition;
        };
        
        struct state_t {
            std::vector<rule_t> _rules;
            std::vector<size_t> _pre;
        };
        
        struct tos_t {
            std::vector<T> _tos; // TODO: some symbolic representation would be better here
            std::vector<T> _stack; // TODO: some symbolic representation would be better here
            int _in_waiting = 0;

            bool empty_tos() const {
                return _tos.empty();
            } 

            bool active(const tos_t& prev, const rule_t& rule, const std::vector<T>& all_labels)
            {
                if(rule._precondition.empty())
                    return false;
                else if(rule._precondition.wildcard())
                {
                    return true;
                }
                else
                {
                    auto rit = rule._precondition.labels().begin();
                    bool match = false;
                    for(auto& s : prev._tos)
                    {
                        while(rit != std::end(rule._precondition.labels()) && *rit < s) ++rit;
                        if(rit == std::end(rule._precondition.labels())) break;
                        if(*rit == s){ match = true; break; }
                    }
                    return match;
                }
                return true;
            }
            
            bool forward_stack(const tos_t& prev, const std::vector<T>& all_labels) {
                bool changed = false;
                if(_stack.size() != all_labels.size())
                {
                    if(prev._stack.size() == all_labels.size())
                    {
                        _stack = all_labels;
                        changed = true;
                    }
                    else
                    {
                        auto lid = _stack.begin();
                        auto pid = prev._stack.begin();
                        while(pid != std::end(prev._stack))
                        {
                            while(lid != _stack.end() && *lid < *pid) ++lid;
                            if(lid != _stack.end() && *lid == *pid) { ++pid; continue;}
                            if(lid == _stack.end()) break;
                            auto oid = pid;
                            while(pid != std::end(prev._stack) && *pid < *lid) ++pid;
                            lid = _stack.insert(lid, oid, pid);
                            changed = true;
                        }
                        _stack.insert(lid, pid, std::end(prev._stack));
                    }
                }   
                return changed;
            }
            
            bool merge_pop(const tos_t& prev, const rule_t& rule, bool dual_stack, const std::vector<T>& all_labels)
            {
                if(!active(prev, rule, all_labels))
                {
                    return false;
                }
                bool changed = false;
                if(!dual_stack)
                {
                    _tos = all_labels;
                }
                else
                {
                    // move stack->stack
                    changed |= forward_stack(prev, all_labels);
                    // move stack -> TOS
                    if(_tos.size() != all_labels.size())
                    {
                        if(prev._stack.size() == all_labels.size())
                        {
                            _tos = all_labels;
                            changed = true;
                        }
                        else
                        {
                            auto it = _tos.begin();
                            for(auto s : prev._stack)
                            {
                                while(it != std::end(_tos) && *it < s) ++it;
                                if(it != std::end(_tos) && *it == s) continue;
                                it = _tos.insert(it, s);
                                changed = true;
                            }
                        }
                    }
                }
                return changed;
            }

           
            bool merge_noop(const tos_t& prev, const rule_t& rule, bool dual_stack, const std::vector<T>& all_labels)
            {
                if(rule._precondition.empty()) return false;
                bool changed = false;
                {
                    auto iit = _tos.begin();
                    for(auto& symbol : rule._precondition.wildcard() ? prev._tos : rule._precondition.labels())
                    {
                        while(iit != _tos.end() && *iit < symbol) ++iit;
                        if(iit != _tos.end() && *iit == symbol) { ++iit; continue; }
                        changed = true;
                        iit = _tos.insert(iit, symbol);
                        ++iit;
                    }
                }
                if(dual_stack)
                {
                    changed |= forward_stack(prev, all_labels);
                }
                return changed;
            }
            
            bool merge_swap(const tos_t& prev, const rule_t& rule, bool dual_stack, const std::vector<T>& all_labels)
            {
                if(!active(prev, rule, all_labels)) return false; // we know that there is a match!
                bool changed = false;

                {
                    auto lb = std::lower_bound(_tos.begin(), _tos.end(), rule._op_label);
                    if(lb == std::end(_tos) || *lb != rule._op_label)
                    {
                        changed = true;
                        _tos.insert(lb, rule._op_label);
                    }
                }
                if(dual_stack)
                {
                    changed |= forward_stack(prev, all_labels);
                }
                return changed;                
            }
            
            bool merge_push(const tos_t& prev, const rule_t& rule, bool dual_stack, const std::vector<T>& all_labels)
            {
                // similar to swap!
                bool changed = merge_swap(prev, rule, dual_stack, all_labels);
                if(dual_stack)
                {
                    // but we also push all TOS labels down
                    if(_stack.size() != all_labels.size())
                    {
                        if(prev._tos.size() == all_labels.size())
                        {
                            changed = true;
                            _stack = all_labels;
                        }
                        else
                        {
                            auto it = _stack.begin();
                            for(auto& s : prev._tos)
                            {
                                while(it != _stack.end() && *it < s) ++it;
                                if(it != std::end(_stack) && *it == s) continue;
                                it = _stack.insert(it, s);
                                changed = true;
                            }
                        }
                    }
                }
                return changed;
            }
        };
        
        void forwards_prune() {
            // lets do a forward/backward pruning first
            std::queue<size_t> waiting;
            std::vector<bool> seen(_states.size());
            waiting.push(0);
            seen[0] = true;
            while(!waiting.empty())
            {
                // forward
                auto el = waiting.front();
                waiting.pop();
                for(auto& r : _states[el]._rules)
                {
                    if(!seen[r._to])
                    {
                        waiting.push(r._to);
                        seen[r._to] = true;
                    }
                }
            }
            for(size_t s = 0; s < _states.size(); ++s)
            {
                if(!seen[s])
                {
                    _states[s]._rules.clear();
                    _states[s]._pre.clear();
                }
            }            
        }
        
        void backwards_prune(){
            std::queue<size_t> waiting;
            std::vector<bool> seen(_states.size());
            waiting.push(0);
            seen[0] = true;
            while(!waiting.empty())
            {
                // forward
                auto el = waiting.front();
                waiting.pop();
                for(auto& s2 : _states[el]._pre)
                {
                    if(!seen[s2])
                    {
                        waiting.push(s2);
                        seen[s2] = true;
                    }
                }
            }
            for(size_t s = 0; s < _states.size(); ++s)
            {
                if(!seen[s])
                {
                    _states[s]._rules.clear();
                    _states[s]._pre.clear();
                }
            }
        }
    public:

        void target_tos_prune(){
            for(size_t s = 1; s < _states.size(); ++s)
            {
                std::unordered_set<T> usefull_tos;
                bool cont = false;
                bool any_dot = false;
                for(auto& r : _states[s]._rules)
                {
                    if(!r._precondition.wildcard())
                    {
                        if(!usefull_tos.size() == _all_labels.size())
                            usefull_tos.insert(r._precondition.labels().begin(), r._precondition.labels().end());
                    }
                    else
                    {
                        if(usefull_tos.size() != _all_labels.size())
                            usefull_tos.insert(_all_labels.begin(), _all_labels.end());
                    }
                    if(usefull_tos.size() == _all_labels.size() && any_dot) {
                        cont = true;
                        break;
                    }
                }
                
                if(cont) continue;
//                std::cerr << "S" << s << std::endl;
                for(auto& pres : _states[s]._pre)
                {
//                    std::cerr << "\t PRE S" << pres << std::endl;
                    auto& state = _states[pres];
                    for(auto& r : state._rules)
                    {
                        if(r._to == s)
                        {
                            switch(r._operation)
                            {
                                case SWAP:
                                case PUSH:
                                    if(!r._precondition.labels().empty() && usefull_tos.count(r._op_label) == 0)
                                    {
                                        r._precondition.clear();
                                    }
                                    break;
                                case NOOP:
                                {
                                    r._precondition.noop_pre_filter(usefull_tos);
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
        
        std::pair<size_t, size_t> reduce(int aggresivity)
        {
            size_t cnt = size();

            forwards_prune();
            backwards_prune();
            std::queue<size_t> waiting;            
            if(aggresivity == 0) 
            {
                size_t after_cnt = size();
                return std::make_pair(cnt, after_cnt);
            }
            auto ds = (aggresivity >= 2);
            std::vector<tos_t> approximation(_states.size());

            // initialize
            for(auto& r : _states[0]._rules)
            {
                if(r._to == 0) continue;
                if(r._precondition.empty()) continue;
                auto& ss = approximation[r._to];
                if(ss._in_waiting == 0)
                {
                    waiting.push(r._to);
                    ss._in_waiting = -1;
                }
                assert(r._operation == PUSH);
                auto lb = std::lower_bound(ss._tos.begin(), ss._tos.end(), r._op_label);
                if(lb == std::end(ss._tos) || *lb != r._op_label)
                    ss._tos.insert(lb, r._op_label);
            }
                      
            // saturate
            while(!waiting.empty())
            {
                auto el = waiting.front();
                waiting.pop();
                auto& ss = approximation[el];
                auto& state = _states[el];
                auto fit = state._rules.begin();
                while(fit != std::end(state._rules))
                {
                    if(fit->_to == 0) { ++fit; continue; }
                    if(fit->_precondition.empty()) { ++fit; continue; }
                    auto& to = approximation[fit->_to];
                    // handle dots!
                    bool change = false;
                    switch(fit->_operation)
                    {
                        case POP:
                            change = to.merge_pop(ss, *fit, ds, _all_labels);
                            break;
                        case NOOP:
                            change = to.merge_noop(ss, *fit, ds, _all_labels);
                            break;
                        case PUSH:
                            change = to.merge_push(ss, *fit, ds, _all_labels);
                            break;
                        case SWAP:
                            change = to.merge_swap(ss, *fit, ds, _all_labels);
                            break;
                        default:
                            throw base_error("Unknown PDA operation");
                            break;
                    }
                    if(change || ss._in_waiting == -1)
                    {
                        if(to._in_waiting >= 0)
                        {
                            to._in_waiting = to._in_waiting == 0 ? -1 : -2;
                            waiting.push(fit->_to);
                        }
                    }
                    ss._in_waiting = 1;
                    ++fit;
                }
            }
            
            // DO PRUNING!
            for(size_t i = 1; i < _states.size(); ++i)
            {
                auto& app = approximation[i];
                auto& state = _states[i];
                size_t br = 0;
/*                std::cerr << "[" << i << "] \n";
                std::cerr << "\t" << (app._top_dot ? "DOT + [" : "[");
                if(app._tos.size() == _all_labels.size())
                    std::cerr << "ALL";
                else
                {
                    for(auto& l : app._tos)
                    {
                        std::cerr << l << ", ";
                    }
                }
                std::cerr << "]\n";
                std::cerr << "\t" << (app._stack_dot ? "DOT + [" : "[");
                if(app._stack.size() == _all_labels.size())
                    std::cerr << "ALL";
                else
                {
                    for(auto& l : app._stack)
                    {
                        std::cerr << l << ", ";
                    }
                }
                std::cerr << "]\n";*/
                for(size_t r = 0; r < state._rules.size(); ++r)
                {
                    // check rule
                    auto& rule = state._rules[r];
/*                    std::cerr << "PRE \n\t";
                    for(auto& p : rule._precondition.labels())
                    {
                        std::cerr << p << ",";
                    }
                    std::cerr << "\n\t";
                    if(rule._precondition.wildcard())
                            std::cerr << "Wildcard\n\t";
                    for(auto& l : rule._precondition.labels())
                        std::cerr << l << ",";
                    std::cerr << std::endl;*/
                    if(rule._to == 0 || rule._precondition.intersect(app, _all_labels))
                    {
/*                        std::cerr << "Keep rule on [[" << i << "]] with oplabel " << rule._op_label << " AND OP " << rule._operation << std::endl;
                        if(rule._precondition.wildcard())
                            std::cerr << "\tWildcard\n";
                        std::cerr << "\t" << rule._precondition.labels().size() << std::endl;*/
                        if(br != r)
                        {
                            std::swap(state._rules[br], state._rules[r]);
                        }
                        ++br;
                    }
/*                    else
                    {
                        std::cerr << "Remove rule on [[" << i << "]] with oplabel " << rule._op_label << " AND OP " << rule._operation << std::endl;
                    }*/
                }
                state._rules.resize(br);
                if(state._rules.empty())
                {
//                    std::cerr << "CLEAR S" << i << " !!" << std::endl;
                    state._pre.clear();
                }
/*                else
                {
                    std::cerr << "NS S" << i << std::endl; 
                }*/
            }
            
            forwards_prune();
            backwards_prune();
            if(aggresivity == 3)
            {
                // it could potentially work cyclic; not sure if it has any effect.
                target_tos_prune();
                forwards_prune();
                backwards_prune();
            }
            
            size_t after_cnt = size();
            return std::make_pair(cnt, after_cnt);
        }
       
        size_t size() const {
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
                    cnt += r._precondition.wildcard() ? labelset().size() : r._precondition.labels().size();
                }
            }
            return cnt;
        }
        
        const std::vector<state_t>& states() const
        {
            return _states;
        }
        
        const std::vector<T>& labelset() const
        {
            return _all_labels;
        }
        
    protected:
        friend class PDAFactory<T>;
        
        PDA(std::unordered_set<T>& all_labels) 
        {
            _all_labels.insert(_all_labels.end(), all_labels.begin(), all_labels.end());
            std::sort(_all_labels.begin(), _all_labels.end());
        }
        
        void add_rules(size_t from, size_t to, op_t op, bool negated, const std::vector<T>& labels, bool negated_pre, const std::vector<T>& pre)
        {
            if(negated)
            {
                auto lid = labels.begin();
                auto ait = _all_labels.begin();
                for(; lid != labels.end(); ++lid)
                {
                    while(*ait < *lid && ait != std::end(_all_labels))
                    {
                        add_rule(from, to, op, *ait, negated_pre, pre);
                        ++ait;
                    }
                    assert(*ait == *lid);
                    ++ait;
                    assert(ait == std::end(_all_labels) || *ait != *lid);
                }
                for(; ait != std::end(_all_labels); ++ait)
                    add_rule(from, to, op, *ait, negated_pre, pre);
            }
            else
            {
                for(auto& s : labels)
                {
                    add_rule(from, to, op, s, negated_pre, pre);
                }
            }
        }
                
        void add_rule(size_t from, size_t to, op_t op, T label, bool negated, const std::vector<T>& pre)
        {
            auto mm = std::max(from, to);
            if(mm >= _states.size())
                _states.resize(mm+1);
            rule_t r;
            r._to = to;
            r._op_label = label;
            r._operation = op;
            auto& rules = _states[from]._rules;
            auto lb = std::lower_bound(rules.begin(), rules.end(), r);
            if(lb == std::end(rules) || *lb != r)
                lb = rules.insert(lb, r); // TODO this is expensive. Use lists?

            lb->_precondition.merge(negated, pre, _all_labels);
            auto& prestate = _states[to]._pre;
            auto lpre = std::lower_bound(prestate.begin(), prestate.end(), from);
            if(lpre == std::end(prestate) || *lpre != from)
                prestate.insert(lpre, from);
        }

        std::vector<state_t> _states;
        std::vector<T> _all_labels;

    };
    
    template<typename T>
    void PDA<T>::pre_t::merge(bool negated, const std::vector<T>& other, const std::vector<T>& all_labels)
    {
        if(_wildcard) return;
        if(negated && other.empty())
        {
            _wildcard = true;
            _labels.clear();
            return;
        }
                
        if(!negated)
        {
            auto lid = _labels.begin();
            for(auto fit = other.begin(); fit != other.end(); ++fit)
            {
                while(lid != _labels.end() && *lid < *fit) ++lid;
                if(lid == _labels.end())
                {
                    _labels.insert(_labels.end(), fit, other.end());
                    break;
                }
                if(*lid == *fit) continue;
                lid = _labels.insert(lid, *fit);
                ++lid;
            }            
        }
        else
        {
            auto lid = _labels.begin();
            auto ait = all_labels.begin();
            for(auto fit = other.begin(); fit != other.end(); ++fit)
            {
                auto nm = ait;
                while(nm != std::end(all_labels) && *nm < *fit) ++nm;
                // assert(*nm == *fit); TODO, check if this is ok
                for(;ait != nm; ++ait)
                {
                    while(lid != std::end(_labels) && *lid < *ait) ++lid;
                    if(lid == std::end(_labels))
                    {
                        _labels.insert(_labels.end(), ait, nm);
                        lid = std::end(_labels);
                        break;
                    }
                    else if(*ait == *lid) continue;
                    else
                    {
                        assert(*ait < *lid);
                        lid = _labels.insert(lid, *ait);
                        ++lid;
                    }
                }

            }
            for(;ait != std::end(all_labels); ++ait)
            {
                while(lid != std::end(_labels) && *lid < *ait) ++lid;                        
                if(lid == std::end(_labels))
                {
                    _labels.insert(_labels.end(), ait, std::end(all_labels));
                    lid = std::end(_labels);
                    break;
                }
                else if(*ait == *lid) continue;
                else
                {
                    assert(*ait < *lid);
                    lid = _labels.insert(lid, *ait);
                    ++lid;
                }
            }
        }
        
        if(_labels.size() == all_labels.size())
        {
            _wildcard = true;
            _labels.clear();
        }       
    }
    
    template<typename T>
    void PDA<T>::pre_t::noop_pre_filter(const std::unordered_set<T>& usefull)
    {
        if(_wildcard)
        {
            _labels.insert(_labels.begin(), usefull.begin(), usefull.end());
            _wildcard = false;
        }
        else
        {
            auto it = _labels.begin();
            auto wit = it;
            auto uit = usefull.begin();
            while(it != _labels.end())
            {
                while(uit != std::end(usefull) && *uit < *it) ++uit;
                if(uit != std::end(usefull) && *uit == *it)
                {
                    *wit = *it;
                    ++wit;
                    ++it;
                }
                else
                {
                    ++it;
                }
            }
            if(wit != it)
            {
                _labels.resize(wit - _labels.begin());
            }
        }
    }
    
    template<typename T>
    bool PDA<T>::pre_t::intersect(const tos_t& tos, const std::vector<T>& all_labels)
    {
        if(tos._tos.size() == all_labels.size())
        {
//            std::cerr << "EMPY 1" << std::endl;
            return !empty();
        }
        if(_wildcard)
        {
            if(tos._tos.size() == all_labels.size())
                return empty();
            else
            {
                _labels = tos._tos;
                _wildcard = false;
            }
        }
        else
        {
            auto fit = tos._tos.begin();
            size_t bit = 0;
            for(size_t nl = 0; nl < _labels.size(); ++nl)
            {
                while(fit != std::end(tos._tos) && *fit < _labels[nl]) ++fit;
                if(fit == std::end(tos._tos)) break;
                if(*fit == _labels[nl])
                {
                    _labels[bit] = _labels[nl];
                    ++bit;
                }
            }
            _labels.resize(bit);
            assert(_labels.size() != all_labels.size());
        }
        return !empty();
    }
}
#endif /* PDA_H */

