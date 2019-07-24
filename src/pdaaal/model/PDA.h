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
        struct pre_t {
            bool _wildcard = false;
            std::vector<T> _labels;
            void merge(bool negated, const std::vector<T>& other, const std::vector<T>& all_labels);
            bool empty() const {
                return !_wildcard && _labels.empty();
            }
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
                if(_to != _to)
                    return _to < other._to;
                if(_operation != other._operation)
                    return _operation < other._operation;
                if(_dot_label != other._dot_label)
                    return _dot_label < other._dot_label;
                return _op_label < other._op_label;
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
        
    public:
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
    
}
#endif /* PDA_H */

