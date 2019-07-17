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

namespace pdaaal {

    template<typename T = char>
    class NFA {
    public:

        struct state_t;

        struct edge_t {
            // we already know that we will have many more symbols than destinations
            bool _epsilon = false;
            bool _negated = false;
            std::vector<T> _symbols;
            state_t* _destination;
            edge_t(bool negated, std::unordered_set<T>& symbols, state_t* dest)
                    : _negated(negated), _symbols(symbols.begin(), symbols.end()), _destination(dest) {};
            edge_t(state_t* dest, bool epsilon = false)
                    : _epsilon(epsilon), _negated(true), _destination(dest) {};
            edge_t(const edge_t& other) = default;
        };
        
        struct state_t {
            std::vector<edge_t> _edges;
            bool _accepting = false;
            state_t(bool accepting) : _accepting(accepting) {};
            state_t(const state_t& other) = default;
        };
        
    public:

        NFA(bool initially_accepting = true) {
            _states.emplace_back(std::make_unique<state_t>(initially_accepting));
        }

        NFA(std::unordered_set<T>&& initial_accepting, bool negated = false) {
            if (initial_accepting.size() > 0 || negated) {
                _states.emplace_back(std::make_unique<state_t>(false));
                _states.emplace_back(std::make_unique<state_t>(true));
                _initial.push_back(_states[0].get());
                _accepting.push_back(_states[1].get());
                _initial.back()->_edges.emplace_back(negated,initial_accepting,_accepting.back());
            }
        }
        NFA(NFA&&) = default;
        NFA& operator=(NFA&&) = default;
        NFA(const NFA& other )
        {
            (*this) = other;
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
                    sa->_edges.emplace_back(s, true);
                }
                sa->_accepting = false;
            }
            _accepting = std::move(other._accepting);
        }
        
        void dot_extend() 
        {
            _states.emplace_back(std::make_unique<state_t>(false));
            _states.emplace_back(std::make_unique<state_t>(true));
            _initial.push_back(_states[0].get());
            _accepting.push_back(_states[1].get());
            _initial.back()->_edges.emplace_back(_accepting.back(), false);
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
                        s->_edges.emplace_back(si, true);
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
                initial->_edges.emplace_back(i, true);
            }
            for(auto& i : other._initial)
            {
                initial->_edges.emplace_back(i, true);
            }            
            _initial = {initial};
            _accepting.insert(_accepting.end(), other._accepting.begin(), other._accepting.end());
        }        

    private:
        const static std::vector<state_t*> empty;
        std::vector<std::unique_ptr<state_t>> _states;
        std::vector<state_t*> _initial;
        std::vector<state_t*> _accepting;

    };


}

#endif /* NFA_H */

