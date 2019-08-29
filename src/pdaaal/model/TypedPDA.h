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

namespace pdaaal {
    template<typename T>
    class PDAFactory;

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
        
        T get_symbol(size_t i) {
            T res;
            for(auto& l : _label_map)
            {
                if(l.second == i)
                {
                    res = l.first;
                    break;
                }
            }
            return res;
        }
        

    protected:
        friend class PDAFactory<T>;

        TypedPDA(std::unordered_set<T>& all_labels) {
            std::set<T> sorted(all_labels.begin(), all_labels.end());
            size_t id = 0;
            for(auto& l : sorted)
            {
                _label_map[l] = id;
                ++id;
            }
        }

        void add_rules(size_t from, size_t to, op_t op, bool negated, const std::vector<T>& labels, bool negated_pre, const std::vector<T>& pre) {
            auto tpre = encode_pre(pre);
            if (negated) {
                size_t last = 0;
                for(auto& l : labels)
                {
                    assert(_label_map.count(l) == 1);
                    auto res = _label_map.find(l);
                    assert(res != std::end(_label_map));
                    for(; last < res->second; ++last)
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
        
        uint32_t find_labelid(op_t op, T label)
        {
            if(op != POP && op != NOOP)
            {
                auto res = _label_map.find(label);
                if(res != std::end(_label_map))
                {
                    return res->second;
                }
                else
                {
                    throw base_error("Couldnt find label during construction");
                }
            }    
            return std::numeric_limits<uint32_t>::max();
        }

        std::vector<uint32_t> encode_pre(const std::vector<T>& pre)
        {
            std::vector<uint32_t> tpre(pre.size());
            for(size_t i = 0; i < pre.size(); ++i)
            {
                auto& p = pre[i];
                assert(_label_map.count(p) == 1);
                auto res = _label_map.find(p);
                assert(res != std::end(_label_map));
                tpre[i] = res->second;
            }     
            return tpre;
        }
        
        std::unordered_map<T, uint32_t> _label_map;

    };
}
#endif /* TPDA_H */

