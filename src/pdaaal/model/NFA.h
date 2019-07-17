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

#include <inttypes.h>
#include <vector>
#include <unordered_set>

namespace pdaaal {

    template<typename T = char>
    class NFA {
    public:
        NFA(bool initially_accepting = true) {}
        NFA(std::vector<T>&& initial_accepting, bool negated = false) {}
        NFA(std::unordered_set<T>&& initial_accepting, bool negated = false) {}
        NFA(const NFA&) = default;
        NFA(NFA&&) = default;
        NFA& operator=(const NFA&) = default;
        NFA& operator=(NFA&&) = default;


        // construction from regex
        void dot_extend(NFA& other)
        {
            
        }
        
        void plus_extend(NFA& other)
        {
            
        }
        
        void star_extend(NFA& other)
        {
            
        }
        
        void and_extend(NFA& other)
        {
            
        }
        
        void or_extend(NFA& other)
        {
            
        }
        
        struct edge_t {
            T _symbol;
            std::vector<size_t> _destinations;
        };
        struct state_t {
            std::vector<edge_t> _edges;
            bool _accepting = false;
            size_t _id = 0;
            const std::vector<size_t>& next(T symbol) const;
        };
        private:
            const static std::vector<size_t> empty;
            std::vector<state_t> _states;
            std::vector<size_t> _initial;
            std::vector<size_t> _accepting;

    };

    template<typename T>
    const std::vector<size_t>& NFA<T>::state_t::next(T symbol) const
    {
        for(auto& e : _edges)
        {
            if(e._symbol == symbol)
                return e._edges;
        }
        return empty;
    }
}

#endif /* NFA_H */

