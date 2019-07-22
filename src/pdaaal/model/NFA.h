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
 * File:   NFA.h
 * Author: Peter G. Jensen <root@petergjoel.dk>
 *
 * Created on July 16, 2019, 5:39 PM
 */

#ifndef NFA_H
#define NFA_H

#include "utils/errors.h"

#include <inttypes.h>
#include <vector>
#include <unordered_set>
#include <memory>
#include <unordered_map>
#include <ostream>
#include <functional>
#include <iostream>


namespace pdaaal {

    template<typename T = char>
    class NFA {
        // TODO: Some trivial optimizations to be had here.
    public:

        struct state_t;

        struct edge_t {
            // we already know that we will have many more symbols than destinations
            bool _epsilon = false;
            bool _negated = false;
            std::vector<T> _symbols;
            state_t* _destination;
            edge_t(bool negated, std::unordered_set<T>& symbols, state_t* dest, state_t* source, size_t id)
                    : _negated(negated), _symbols(symbols.begin(), symbols.end()), _destination(dest) 
            {
                _destination->_backedges.emplace_back(source, id);
                std::sort(_symbols.begin(), _symbols.end());
            };
            edge_t(state_t* dest, state_t* source, size_t id, bool epsilon = false)
                    : _epsilon(epsilon), _negated(!epsilon), _destination(dest) 
            {
                _destination->_backedges.emplace_back(source, id);
            };
            edge_t(const edge_t& other) = default;
            bool empty(size_t n) const {
                if(!_negated)
                    return _symbols.empty();
                return _symbols.size() == n;
            }
            bool wildcard(size_t n) const {
                if(_negated)
                    return _symbols.empty();
                else //if(!_negated)
                    return _symbols.size() == n;
            }
        };
        
        struct state_t {
            std::vector<edge_t> _edges;
            std::vector<std::pair<state_t*,size_t>> _backedges;
            bool _accepting = false;
            state_t(bool accepting) : _accepting(accepting) {};
            state_t(const state_t& other) = default;
            bool has_non_epsilon() const 
            {
                for(auto& e : _edges)
                {
                    if(!e._negated && !e._symbols.empty())
                        return true;
                    else if(e._negated) return true;
                }
                return false;
            }
            std::vector<T> prelabels(const std::unordered_set<T>& all_labels) const
            {
                std::unordered_set<const state_t*> seen{this};
                std::vector<const state_t*> waiting{this};
                std::vector<T> labels;
                while(!waiting.empty())
                {
                    auto s = waiting.back();
                    waiting.pop_back();
                    
                    for(auto& e : s->_backedges)
                    {
                        if(seen.count(e.first) == 0)
                        {
                            seen.insert(e.first);
                            waiting.push_back(e.first);
                        }
                        auto& edge = e.first->_edges[e.second];
                        if(edge._negated)
                        {
                            if(edge._symbols.empty())
                            {
                                labels.clear();
                                labels.insert(labels.end(), all_labels.begin(), all_labels.end());
                                return labels;
                            }
                            else
                            {
                                for(auto& l : all_labels)
                                {
                                    auto lb = std::lower_bound(edge._symbols.begin(), edge._symbols.end(), l);
                                    if(lb == std::end(edge._symbols) || *lb != l)
                                    {
                                        auto lb2 = std::lower_bound(labels.begin(), labels.end(), l);
                                        if(lb2 == std::end(labels) || *lb2 != l)
                                            labels.insert(lb2, l);
                                    }
                                }
                            }
                        }
                        else
                        {
                            for(auto l : edge._symbols)
                            { // sorted, so we can optimize here of we want to 
                                auto lb2 = std::lower_bound(labels.begin(), labels.end(), l);
                                if(lb2 == std::end(labels) || *lb2 != l)
                                    labels.insert(lb2, l);
                            }
                        }
                    }
                }
                return labels;
            }
        };
        
    public:

        NFA(bool initially_accepting = true) {
            _states.emplace_back(std::make_unique<state_t>(initially_accepting));
            _accepting.push_back(_states.back().get());
            _initial.push_back(_states.back().get());
        }

        NFA(std::unordered_set<T>&& initial_accepting, bool negated = false) {
            if (initial_accepting.size() > 0 || negated) {
                _states.emplace_back(std::make_unique<state_t>(false));
                _initial.push_back(_states[0].get());
                _states.emplace_back(std::make_unique<state_t>(true));
                _accepting.push_back(_states[1].get());
                _initial.back()->_edges.emplace_back(negated,initial_accepting,_accepting.back(),_initial.back(), 0);
            }
        }
        
        void compile()
        {
            std::sort(_accepting.begin(), _accepting.end());
            std::sort(_initial.begin(), _initial.end());
            follow_epsilon(_initial);
            std::sort(_states.begin(), _states.end());
        }
        
        NFA(NFA&&) = default;
        NFA& operator=(NFA&&) = default;
        NFA(const NFA& other )
        {
            (*this) = other;
        }

        template<typename C>
        static void follow_epsilon(std::vector<C>& states)
        {
            std::vector<C> waiting = states;
            while(!waiting.empty())
            {
                auto s = waiting.back();
                waiting.pop_back();
                for(auto& e : s->_edges)
                {
                    if(e._epsilon)
                    {
                        auto lb = std::lower_bound(states.begin(), states.end(), e._destination);
                        if(lb == std::end(states) || *lb != e._destination)
                        {
                            states.insert(lb, e._destination);
                            waiting.push_back(e._destination);
                        }
                    }
                }
            }
            auto res = std::remove_if(states.begin(), states.end(), [](const state_t* s){ return !(s->_accepting || s->has_non_epsilon());});
            states.erase(res, states.end());
        }
        
        static std::vector<const state_t*> successor(std::vector<const state_t*>& states, const T& label)
        {
            std::vector<const state_t*> next;
            for(auto& s : states)
            {
                for(auto& e : s->_edges)
                {
                    auto lb = std::lower_bound(e._symbols.begin(), e._symbols.end(), label);
                    auto match = lb != std::end(e._symbols) && *lb == label;
                    if(match == e._negated)
                    {
                        auto slb = std::lower_bound(next.begin(), next.end(), e._destination);
                        if(slb == std::end(next) || *slb != e._destination)
                            next.insert(slb, e._destination);
                    }
                }
            }
            follow_epsilon(next);
            return next;
        }
        
        NFA& operator=(const NFA& other)
        {
            std::unordered_map<state_t*, state_t*> indir;
            for(auto& s : other._states)
            {
                _states.emplace_back(std::make_unique<state_t>(*s));
                indir[s.get()] = _states.back().get();
            }     
            // fix links
            for(auto& s : _states)
            {
                for(auto& e : s->_edges)
                {
                    e._destination = indir[e._destination];
                }
            }
            for(auto& s : other._accepting)
            {
                _accepting.push_back(indir[s]);
            }
            for(auto& s : other._initial)
            {
                _initial.push_back(indir[s]);
            }
            return *this;
        }


        // construction from regex
        void concat(NFA&& other)
        {
            for(auto& s : other._states)
                _states.emplace_back(s.release());
            for(auto& sa : _accepting)
            {
                for(auto& s : other._initial)
                {
                    auto id = sa->_edges.size();
                    sa->_edges.emplace_back(s, sa, id, true);
                }
                sa->_accepting = false;
            }
            _accepting = std::move(other._accepting);
        }
        
        void question_extend()
        {
            for(auto s : _initial)
            {
                if(!s->_accepting)
                {
                    s->_accepting = true;
                    _accepting.push_back(s);
                }
            }
        }
        
        void plus_extend() {
            for(auto s : _accepting)
            {
                for(auto si : _initial)
                {
                    bool found = false;
                    for(auto& e : s->_edges)
                    {
                        if(e._destination == si)
                            e._epsilon = true;
                    }
                    if(!found)
                    {
                        auto id = s->_edges.size();
                        s->_edges.emplace_back(si, s, id, true);
                    }
                }
            }
        }

        void star_extend() {
            plus_extend();
            question_extend();
        }

        void and_extend(NFA&& other) {
            // prune? Powerset?
            throw base_error("conjunction for NFAs are not yet implemented");
        }

        void or_extend(NFA&& other) 
        {
            for(auto& s : other._states)
                _states.emplace_back(s.release());            
            _states.emplace_back(std::make_unique<state_t>(false));
            auto initial = _states.back().get();
            for(auto& i : _initial)
            {
                initial->_edges.emplace_back(i, initial, 0, true);
            }
            for(auto& i : other._initial)
            {
                initial->_edges.emplace_back(i, initial, 0, true);
            }            
            _initial = {initial};
            _accepting.insert(_accepting.end(), other._accepting.begin(), other._accepting.end());
        }       
        
        void to_dot(std::ostream& out, std::function<void(std::ostream&, const T&)> printer = [](auto& s, auto& e){ s << e ;}) const
        {
            out << "digraph NFA {\n";
            for(auto& s : _states)
            {
                out << "\"" << s.get() << "\" [shape=";
                if(s->_accepting)
                    out << "double";
                out << "circle];\n";
                for(const edge_t& e : s->_edges)
                {
                    out << "\"" << s.get() << "\" -> \"" << e._destination << "\" [ label=\"";
                    if(e._negated && !e._symbols.empty())
                    {
                        out << "^";
                    }
                    if(!e._symbols.empty())
                    {
                        out << "\\[";
                        bool first = true;
                        for(auto& s : e._symbols)
                        {
                            if(!first)
                                out << ", ";
                            first = false;
                            printer(out, s);
                        }
                        out << "\\]";
                    }
                    if(e._symbols.empty() && e._negated)
                        out << "*";
                    if(e._epsilon)
                    {
                        if(!e._symbols.empty() || e._negated) out << " ";
                        out << u8"𝜀";
                    }
                    
                    out << "\"];\n";
                }
            }
            for(auto& i : _initial)
            {
                out << "\"I" << i << "\" -> \"" << i << "\";\n";
                out << "\"I" << i << "\" [style=invisible];\n";
            }
            
            out << "}\n";
        }

        const std::vector<state_t*>& initial() { return _initial; }
        const std::vector<state_t*>& accepting() { return _accepting; }
        const std::vector<std::unique_ptr<state_t>> states() { return _states; }
        
    private:
        const static std::vector<state_t*> empty;
        std::vector<std::unique_ptr<state_t>> _states;
        std::vector<state_t*> _initial;
        std::vector<state_t*> _accepting;

    };


}

#endif /* NFA_H */

