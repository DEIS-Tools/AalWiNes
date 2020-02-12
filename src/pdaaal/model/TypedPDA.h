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
 * File:   TypedPDA.h
 * Author: Peter G. Jensen <root@petergjoel.dk>
 *
 * Created on July 23, 2019, 1:34 PM
 */

#ifndef TPDA_H
#define TPDA_H

#include "PDA.h"

#include "utils/errors.h"

#include <vector>
#include <queue>
#include <unordered_set>
#include <unordered_map>
#include <set>
#include <cassert>
#include <iostream>
#include <ptrie/ptrie_map.h>

namespace pdaaal {
    template<typename T>
    class PDAFactory;
    template<typename T>
    class PAutomaton;

    template<typename T>
    class TypedPDA : public PDA {
    public:
        struct tracestate_t {
            size_t _pdastate = 0;
            std::vector<T> _stack;
        };
    public:
        virtual size_t number_of_labels() const {
            return _label_map.size();
        }
        
        T get_symbol(size_t i) const {
            T res;
            _label_map.unpack(i, &res);
            return res;
        }
        

    protected:
        friend class PDAFactory<T>;
        friend class PAutomaton<T>;

        TypedPDA(std::unordered_set<T>& all_labels) {
            std::set<T> sorted(all_labels.begin(), all_labels.end());
            for(auto& l : sorted)
            {
#ifndef NDEBUG
                auto r = 
#endif
                _label_map.insert(l);
#ifndef NDEBUG
                assert(r.first);
#endif
            }
        }

        void add_rules(size_t from, size_t to, op_t op, bool negated, const std::vector<T>& labels, bool negated_pre, const std::vector<T>& pre) {
            auto tpre = encode_pre(pre);
            if (negated) {
                size_t last = 0;
                for(auto& l : labels)
                {
                    auto res = _label_map.exists(l);
                    assert(res.first);
                    for(; last < res.second; ++last)
                    {
                        _add_rule(from, to, op, last, negated_pre, tpre);
                    }
                    ++last;
                }
                for(; last < _label_map.size(); ++last)
                {
                    _add_rule(from, to, op, last, negated_pre, tpre);
                }
            } else {
                for (auto& s : labels) {
                    auto lid = find_labelid(op, s);
                    _add_rule(from, to, op, lid, negated_pre, tpre);
                }
            }
        }

        void add_rule(size_t from, size_t to, op_t op, T label, bool negated, const std::vector<T>& pre) {
            auto lid = find_labelid(op, label);
            auto tpre = encode_pre(pre);
            _add_rule(from, to, op, lid, negated, tpre);
        }
        
        uint32_t find_labelid(op_t op, T label) const
        {
            if(op != POP && op != NOOP)
            {
                auto res = _label_map.exists(label);
                if(res.first)
                {
                    return res.second;
                }
                else
                {
                    throw base_error("Couldnt find label during construction");
                }
            }    
            return std::numeric_limits<uint32_t>::max();
        }

        std::vector<uint32_t> encode_pre(const std::vector<T>& pre) const
        {
            std::vector<uint32_t> tpre(pre.size());
            for(size_t i = 0; i < pre.size(); ++i)
            {
                auto& p = pre[i];
                auto res = _label_map.exists(p);
                if(!res.first)
                {
                    std::cerr << (int)p.type() << ", " << (int)p.mask() << ", " << (int)p.value() << std::endl;
                    std::cerr << "SIZE " << _label_map.size() << std::endl;
                    assert(false);
                }
                tpre[i] = res.second;
            }     
            return tpre;
        }
                
        ptrie::set_stable<T> _label_map;

    };
}
#endif /* TPDA_H */

