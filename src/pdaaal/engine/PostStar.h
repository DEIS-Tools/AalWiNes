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
 *  Copyright Peter G. Jensen, all rights reserved.
 */

/* 
 * File:   PostStar.h
 * Author: Peter G. Jensen <root@petergjoel.dk>
 *
 * Created on August 26, 2019, 4:00 PM
 */

#ifndef POSTSTAR_H
#define POSTSTAR_H


#include "pdaaal/model/TypedPDA.h"

#include <limits.h>

namespace pdaaal {

    class PostStar {
    private:
        struct edge_t {
            size_t _from = std::numeric_limits<size_t>::max();
            size_t _to = std::numeric_limits<size_t>::max();
            size_t _id = 0;
            uint32_t _label = std::numeric_limits<uint32_t>::max();
            bool _in_rel = false;
            bool _in_waiting = false;
            edge_t() {};
            edge_t(size_t from, uint32_t label, size_t to) 
            : _from(from), _to(to), _label(label) {};
            bool operator<(const edge_t& other) const 
            {
                if(_from != other._from) return _from < other._from;
                if(_label != other._label) return _label < other._label;
                return _from < other._from;
            }
            
            bool operator==(const edge_t& other) const {
                return _from == other._from && _to == other._to && _label == other._label;
            }
            
            bool operator!=(const edge_t& other) const {
                return !(*this == other);
            }
        };
    public:
        PostStar();

        virtual ~PostStar();

        bool verify(const PDA& pda, bool build_trace);

        template<typename T>
        std::vector<typename TypedPDA<T>::tracestate_t> get_trace(TypedPDA<T>& pda) const;
    private:

    };
    
    template<typename T>
    std::vector<typename TypedPDA<T>::tracestate_t> PostStar::get_trace(TypedPDA<T>& pda) const
    {
        
    }
    
}

#endif /* POSTSTAR_H */

