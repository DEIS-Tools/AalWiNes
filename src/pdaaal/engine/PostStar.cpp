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
 * File:   PostStar.cpp
 * Author: Peter G. Jensen <root@petergjoel.dk>
 * 
 * Created on August 26, 2019, 4:00 PM
 */

#include "PostStar.h"

#include <ptrie/ptrie_stable.h>

#include <stack>
#include <cassert>
#include <memory>
#include <functional>

namespace pdaaal
{

    PostStar::PostStar()
    {

    }

    PostStar::~PostStar()
    {

    }

    bool PostStar::verify(const PDA& pda, bool build_trace)
    {
        // we don't get a P-automaton here, even though that might speed up things significantly!
        // http://www.lsv.fr/Publis/PAPERS/PDF/schwoon-phd02.pdf page 48
        std::vector<std::unique_ptr<edge_t>> edges;

        std::stack<edge_t*> trans;
        std::vector<edge_t*> rel;
        auto dummy = std::make_unique<edge_t>();
        ptrie::set_stable<> stateacts;
        auto npdastates = pda.states().size();
        auto state_id = [&stateacts,npdastates](size_t from, size_t to, uint32_t lbl)
        {
            size_t el[3] = {from, to, lbl};
            static_assert(sizeof(size_t) >= sizeof(uint32_t));
            auto res = stateacts.insert((unsigned char*)&el, sizeof(size_t)*2 + sizeof(uint32_t));
            return res.second + npdastates;
        };
        auto insert_edge = [&edges,&dummy,&trans](size_t from, uint32_t lbl, size_t to, bool add_to_waiting) -> edge_t*
        {
            (*dummy) = edge_t(from, lbl, to);
            auto lb = std::lower_bound(std::begin(edges), std::end(edges), dummy, [](auto& a, auto& b) -> bool { return *a < *b; });
            if (lb == std::end(edges) || *(*lb) != *dummy) {
                lb = edges.insert(lb, std::make_unique<edge_t>(from, lbl, to));
            }
            if(!(*lb)->_in_waiting && add_to_waiting)
            {
                assert(!(*lb)->_in_rel);
                (*lb)->_in_waiting = true;
                trans.push(lb->get());
            }
            return lb->get();
        };

        for (auto& rule : pda.states()[0]._rules) {
            assert(rule._operation == PDA::PUSH);
            edge_t* edge = insert_edge(0, rule._op_label, rule._to, true);
            assert(!edge->_in_waiting);
            trans.emplace(edge);
            edge->_in_waiting = true;
        }

        while (!trans.empty()) {
            auto t = trans.top();
            if (t->_to == 0)
                return true;
            trans.pop();
            if (t->_in_rel) continue;
            t->_in_rel = true;
            t->_in_waiting = false;
            rel.push_back(t);
            if (t->_label != std::numeric_limits<uint32_t>::max()) {
                assert(t._from < pda.states().size());
                for (auto& rule : pda.states()[t->_from]._rules) {
                    if(!rule._precondition.contains(t->_label)) continue;
                    switch (rule._operation) {
                    case PDA::POP:
                        insert_edge(t->_to, std::numeric_limits<uint32_t>::max(), rule._to, true);
                        break;
                    case PDA::SWAP:
                        insert_edge(t->_to, rule._op_label, rule._to, true);
                        break;
                    case PDA::PUSH:
                    {
                        auto ns = state_id(t->_to, rule._to, rule._op_label);
                        insert_edge(rule._to, rule._op_label, ns, true);
                        auto re = insert_edge(ns, t->_label, t->_to, false);
                        re->_in_rel = true;
                        rel.push_back(re);
                        for(auto e : rel)
                        {
                            if(e->_to == ns && e->_label == std::numeric_limits<uint32_t>::max())
                            {
                                insert_edge(e->_from, t->_label, t->_to, true);
                            }
                        }
                    }
                    default:
                        assert(false);
                    }
                }
            }
            else {
                // do some ordering/indexing on rel?
                for (auto o : rel) {
                    if (o->_from == t->_to) {
                        insert_edge(t->_from, o->_label, o->_to, true);
                    }
                }
            }
        }
        return false;
    }

}
