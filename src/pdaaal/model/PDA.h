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

#include <vector>
#include <ptrie_map.h>
#include <queue>

namespace pdaaal {
    template<typename T>
    class PDAFactory;
    
    template<typename T>
    class PDA {
    public:
        using nfastate_t = typename NFA<T>::state_t;
        enum op_t {
            PUSH, 
            POP, 
            SWAP, 
            NOOP
        };
    private:
        struct tos_t;
        struct pre_t {
            bool _wildcard = false;
            std::vector<T> _labels;
            void merge(bool negated, const std::vector<T>& other, const std::vector<T>& all_labels);
            bool empty() const {
                return !_wildcard && _labels.empty();
            }
            bool intersect(const tos_t& tos, const std::vector<T>& all_labels);
        };

        struct rule_t {
            size_t _to;
            op_t _operation = NOOP;
            T _op_label;
            bool _dot_label;
            bool is_terminal() const 
            {
                return _to == 0;
            }
            
            bool operator<(const rule_t& other) const
            {
                if(_dot_label != other._dot_label)
                    return _dot_label < other._dot_label;
                if(_op_label != other._op_label)
                    return _op_label < other._op_label;
                if(_to != _to)
                    return _to < other._to;
                return _operation < other._operation;
            }
            
            bool operator==(const rule_t& other) const
            {
                return _to == other._to && _op_label == other._op_label && _operation == other._operation && _dot_label == other._dot_label;
            }
            
            bool operator!=(const rule_t& other) const {
                return !(*this == other);
            }
            
            pre_t _precondition;
        };
        
        struct state_t {
            std::vector<rule_t> _rules;
            std::vector<size_t> _pre;
            bool _pre_is_dot = true;
        };
        
        struct tos_t {
            std::vector<T> _tos;
            std::vector<T> _stack;
            bool _top_dot = false;
            bool _stack_dot = false;
            bool _in_waiting = false;
            bool wildcard_top(const std::vector<T>& all_labels) const {
                return _top_dot || _tos.size() == all_labels.size();
            }
            bool empty_tos() const {
                return !_top_dot && _tos.empty();
            } 

            bool active(const tos_t& prev, const rule_t& rule, const std::vector<T>& all_labels)
            {
                if(rule._precondition.empty()) return false;
                if(!rule._precondition._wildcard && !prev.wildcard_top(all_labels))
                {
                    auto rit = rule._precondition._labels.begin();
                    bool match = false;
                    for(auto& s : prev._tos)
                    {
                        if(rit == std::end(rule._precondition._labels)) break;
                        if(*rit == s){ match = true; break; }
                        while(rit != std::end(rule._precondition._labels) && *rit < s) ++rit;
                    }
                    return match;
                }
                return true;
            }
            
            void fixup(const std::vector<T>& all_labels)
            {
                if(_tos.size() == all_labels.size())
                {
                    _top_dot = true;
                }
                if(_stack.size() == all_labels.size())
                {
                    _stack_dot = true;
                }                
            }

            bool forward_stack(const tos_t& prev, const std::vector<T>& all_labels) {
                bool changed = false;
                if(prev._stack_dot && !_stack_dot) { _stack_dot = true; changed = true; }
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
                        for(auto& symbol : prev._stack)
                        {
                            while(lid != _stack.end() && *lid < symbol) ++lid;
                            if(lid != _stack.end() && *lid == symbol) continue;
                            lid = _stack.insert(lid, symbol);
                            changed = true;
                        }                            
                    }
                }   
                return changed;
            }
            
            bool merge_pop(const tos_t& prev, const rule_t& rule, bool dual_stack, const std::vector<T>& all_labels, bool expand_dot)
            {
                if(!active(prev, rule, all_labels)) return false;
                bool changed = false;
                if(!dual_stack)
                {
                    changed = !wildcard_top(all_labels);
                    _top_dot = true;
                    if(expand_dot)
                        _tos = all_labels;
                }
                else
                {
                    // move stack->stack
                    changed |= forward_stack(prev, all_labels);
                    // move stack -> TOS
                    if(!_top_dot && prev._stack_dot)
                    {
                        changed = true;
                        _top_dot = true;
                    }
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
                fixup(all_labels);
                return changed;
            }

           
            bool merge_noop(const tos_t& prev, const rule_t& rule, bool dual_stack, const std::vector<T>& all_labels, bool expand_dot)
            {
                if(rule._precondition.empty()) return false;
                bool changed = false;
                if(wildcard_top(all_labels)) 
                {
                    assert(_top_dot);
                    return false;
                }
                if(prev.wildcard_top(all_labels) && rule._precondition._wildcard)
                {
                    changed = !_top_dot;
                    _top_dot = true;
                    if(expand_dot)
                    {
                        _tos = all_labels;
                    }
                    fixup(all_labels);
                    return changed;
                }
                else
                {
                    // one of them is not a wildcard
                    auto iit = _tos.begin();
                    for(auto& symbol : rule._precondition._wildcard ? prev._tos : rule._precondition._labels)
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
                fixup(all_labels);
                return changed;
            }
            
            bool merge_swap(const tos_t& prev, const rule_t& rule, bool dual_stack, const std::vector<T>& all_labels, bool expand_dot)
            {
                if(!active(prev, rule, all_labels)) return false; // we know that there is a match!
                bool changed = false;

                if(rule._dot_label)
                {
                    changed = !_top_dot;
                    _top_dot = true;
                    if(expand_dot)
                        _tos = all_labels;
                }
                else
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
                fixup(all_labels);
                return changed;                
            }
            
            bool merge_push(const tos_t& prev, const rule_t& rule, bool dual_stack, const std::vector<T>& all_labels, bool expand_dot)
            {
                // similar to swap!
                bool changed = merge_swap(prev, rule, dual_stack, all_labels, expand_dot);
                if(dual_stack)
                {
                    // but we also push all TOS labels down
                    if(!_stack_dot && prev._top_dot)
                    {
                        changed = true;
                        _stack_dot = true;
                    }
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
                fixup(all_labels);
                return changed;
            }
        };
    public:
        
        std::pair<size_t, size_t> reduce(int aggresivity)
        {
            size_t cnt = 0;
            for(auto& s : _states)
                cnt += s._rules.size();

            if(aggresivity == 0) 
            {
                return std::make_pair(cnt, cnt);
            }
            auto ds = (aggresivity == 2);
            std::vector<tos_t> approximation(_states.size());
            std::queue<size_t> waiting;
            
            // initialize
            for(auto& r : _states[0]._rules)
            {
                if(r._to == 0) continue;
                if(r._precondition.empty()) continue;
                auto& ss = approximation[r._to];
                if(!ss._in_waiting)
                {
                    std::cerr << "PUSH " << r._to << std::endl;
                    waiting.push(r._to);
                    ss._in_waiting = true;
                }
                assert(r._operation == PUSH);
                if(r._dot_label) ss._top_dot = true;
                else
                {
                    auto lb = std::lower_bound(ss._tos.begin(), ss._tos.end(), r._op_label);
                    if(lb == std::end(ss._tos) || *lb != r._op_label)
                        ss._tos.insert(lb, r._op_label);
                }
            }
            
            // saturate
            while(!waiting.empty())
            {
                auto el = waiting.front();
                waiting.pop();
                std::cerr << "SAT " << el << std::endl;
                std::cerr << waiting.size() << std::endl;
                auto& ss = approximation[el];
                ss._in_waiting = false;
                auto& state = _states[el];
                auto fit = state._rules.begin();
                while(fit != std::end(state._rules))
                {
                    if(fit->_to == 0) { ++fit; continue; }
                    if(fit->_precondition.empty()) { ++fit; continue; }
                    auto& to = approximation[fit->_to];
                    auto& to_state = _states[fit->_to];
                    // handle dots!
                    bool change = false;
                    switch(fit->_operation)
                    {
                        case POP:
                            assert(fit->_dot_label);
                            change = to.merge_pop(ss, *fit, ds, _all_labels, !to_state._pre_is_dot);
                            break;
                        case NOOP:
                            assert(fit->_dot_label);
                            change = to.merge_noop(ss, *fit, ds, _all_labels, !to_state._pre_is_dot);
                            break;
                        case PUSH:
                            change = to.merge_push(ss, *fit, ds, _all_labels, !to_state._pre_is_dot);
                            break;
                        case SWAP:
                            change = to.merge_swap(ss, *fit, ds, _all_labels, !to_state._pre_is_dot);
                            break;
                        default:
                            throw base_error("Unknown PDA operation");
                            break;
                    }
                    if(change)
                    {
                        if(!to._in_waiting)
                        {
                            to._in_waiting = true;
                            waiting.push(fit->_to);
                            std::cerr << "IPUSH " << fit->_to << std::endl;
                        }
                    }
                    ++fit;
                }
            }
            
            // DO PRUNING!
            for(size_t i = 1; i < _states.size(); ++i)
            {
                auto& app = approximation[i];
                auto& state = _states[i];
                if(app.empty_tos())
                {
                    state._pre.clear();
                    state._rules.clear();
                    continue;
                }
                size_t br = 0;
                for(size_t r = 0; r < state._rules.size(); ++r)
                {
                    // check rule
                    auto& rule = state._rules[r];
                    if(rule._precondition.intersect(app, _all_labels))
                    {
                        if(br != r)
                        {
                            state._rules[br] = std::move(state._rules[r]);
                        }
                        ++br;
                    }
                }
                state._rules.resize(br);
            }
            size_t after_cnt = 0;
            for(auto& s : _states)
                after_cnt += s._rules.size();
            return std::make_pair(cnt, after_cnt);

        }
        
        void print_moped(std::ostream& s, std::function<void(std::ostream&, const T&)> labelprinter = [](std::ostream& s, const T& label) {
            s << "l" << label;
        })
        {
            auto write_op = [&labelprinter](std::ostream& s, rule_t& rule, std::string noop)
            {
                assert(rule._to != 0);
                switch(rule._operation)
                {
                    case SWAP:
                        if(rule._dot_label)
                            s << "DOT";
                        else
                        {
                            labelprinter(s, rule._op_label);
                        }
                        break;
                    case PUSH:
                        if(rule._dot_label)
                            s << "DOT ";
                        else
                        {
                            labelprinter(s, rule._op_label);     
                            s << " ";
                        }
                    case NOOP:
                            s << noop;
                        break;
                    case POP:
                    default:
                        break;
                }                
            };

            s << "(I<_>)\n";
            // lets start by the initial transitions
            auto& is = _states[0];
            for(auto& r : is._rules)
            {
                if(r._to != 0)
                {
                    assert(r._operation == PUSH);
                    s << "I<_> --> S" << r._to << "<";
                    write_op(s, r, "_");
                    s << ">\n";
                }
                else
                {
                    assert(r._op_label == NOOP);
                    s << "I<_> --> D<_>\n";
                    return;
                }
            }
            for(size_t sid = 1; sid < _states.size(); ++sid)
            {
                state_t& state = _states[sid];
                for(auto& r : state._rules)
                {
                    if(r._to == 0)
                    {
                        assert(r._operation == NOOP);
                        s << "S" << sid << "<_> --> DONE<_>\n";
                        continue;
                    }
                    if(r._precondition.empty()) continue;
                    if(state._pre_is_dot && (r._precondition._wildcard || r._operation == POP || r._operation == SWAP))
                    {
                        // we can make a DOT -> DOT rule but only if both pre and post are "DOT" -- 
                        // or as the case with POP and SWAP; TOS can be anything 
                        s << "S" << sid << "<DOT> --> ";
                        s << "S" << r._to;
                        s << "<";
                        write_op(s, r, "DOT");
                        s << ">\n";
                    }
                    else if(state._pre_is_dot)
                    {
                        // we effectively settle the state of the DOT symbol here
                        assert(!r._precondition._wildcard);
                        for(auto& symbol : r._precondition._labels)
                        {
                            std::stringstream ss;
                            labelprinter(ss, symbol);
                            s << "S" << sid << "<DOT> --> ";
                            s << "S" << r._to;
                            s << "<";
                            write_op(s, r, ss.str());
                            s << ">\n";
                        }
                    }
                    else
                    {
                        assert(!state._pre_is_dot);
                        bool post_settle = false;
                        auto& symbols = r._precondition._wildcard ? _all_labels : r._precondition._labels;
                        for(auto& symbol : symbols)
                        {
                            std::stringstream ss;
                            std::stringstream postss;
                            labelprinter(ss, symbol);
                            postss << " --> ";
                            postss << "S" << r._to;
                            postss << "<";
                            write_op(postss, r, ss.str());
                            postss << ">\n";
                            s << "S" << sid << "<";
                            labelprinter(s, symbol);
                            s << ">" << postss.str();
                            if(r._precondition._wildcard && (r._operation == NOOP || r._operation == POP || r._operation == PUSH))
                            {
                                // we do not need to settle the DOT yet
                                post_settle = true;
                            }
                            else
                            {
                                // settle the DOT
                                s << "S" << sid << "<DOT> " << postss.str();
                            }
                        }
                        if(post_settle)
                        {
                            s << "S" << sid << "<DOT> --> ";
                            s << "S" << r._to;
                            s << "<";
                            write_op(s, r, "DOT");
                            s << ">\n";
                        }
                    }
                }
            }
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
            if(negated && labels.size() == 0)
            {
                add_wildcard(from, to, op, negated_pre, pre);
            }
            else if(!negated && labels.size() == _all_labels.size())
            {
                add_wildcard(from, to, op, negated_pre, pre);
            }
            else if(negated)
            {
                auto lid = labels.begin();
                auto ait = _all_labels.begin();
                for(; lid != labels.end(); ++lid)
                {
                    while(*ait < *lid)
                        add_rule(from, to, op, false, *ait, negated_pre, pre);
                    assert(*ait == *lid);
                    ++ait;
                    assert(ait == std::end(_all_labels) || *ait != *lid);
                }
                for(; ait != std::end(_all_labels); ++ait)
                    add_rule(from, to, op, false, *ait, negated_pre, pre);
            }
            else
            {
                for(auto& s : labels)
                {
                    add_rule(from, to, op, false, s, negated_pre, pre);
                }
            }
        }
                
        void add_wildcard(size_t from, size_t to, op_t op, bool negated, const std::vector<T>& pre)
        {
            add_rule(from, to, op, true, T{}, negated, pre);
        }
        
        void add_rule(size_t from, size_t to, op_t op, bool is_dot, T label, bool negated, const std::vector<T>& pre)
        {
            if(op == POP && !is_dot)
                throw base_error("Popping-op should be wild-card not supported.");
            if(op == NOOP && !is_dot)
                throw base_error("No-op should be wildcard supported.");
            auto mm = std::max(from, to);
            if(mm >= _states.size())
                _states.resize(mm+1);
            rule_t r;
            r._to = to;
            r._op_label = is_dot ? T{} : label;
            r._operation = op;
            r._dot_label = is_dot;
            if(r._dot_label && label != T{})
            {
                throw base_error("Expected default label when dot-labeled");
            }
            auto& rules = _states[from]._rules;
            auto lb = std::lower_bound(rules.begin(), rules.end(), r);
            if(lb == std::end(rules) || *lb != r)
            {
                lb = rules.insert(lb, r);
                if(!is_dot || (op != PUSH && op != SWAP))
                    _states[to]._pre_is_dot = false;
            }

            lb->_precondition.merge(negated, pre, _all_labels);
            auto& prestate = _states[to]._pre;
            auto lpre = std::lower_bound(prestate.begin(), prestate.end(), from);
            if(lpre == std::end(prestate) || *lpre != from)
                prestate.insert(lpre, from);
        }

        std::vector<state_t> _states;
        std::vector<size_t> _initial;
        std::vector<size_t> _accepting;
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
                assert(*nm == *fit);
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
    bool PDA<T>::pre_t::intersect(const tos_t& tos, const std::vector<T>& all_labels)
    {
        if(tos._tos.size() == all_labels.size())
        {
            return !empty();
        }
        if(_wildcard)
            _labels = tos._tos;
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
                    if(nl != bit)
                        _labels[bit] = _labels[nl];
                    ++bit;
                }
            }
            _labels.resize(bit);
        }
        _wildcard = _labels.size() == all_labels.size();
        if(_wildcard)
            _labels.clear();
        return empty();
    }
}
#endif /* PDA_H */

