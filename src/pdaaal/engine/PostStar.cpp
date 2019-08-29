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
//            std::cerr << "\t(" << from << ", " << lbl << ", " << to << ")" << std::endl;

            (*dummy) = edge_t(from, lbl, to);
            auto lb = std::lower_bound(std::begin(edges), std::end(edges), dummy, [](auto& a, auto& b) -> bool { return *a < *b; });
            if (lb == std::end(edges) || *(*lb) != *dummy) {
                lb = edges.insert(lb, std::make_unique<edge_t>(from, lbl, to));
            }
            if(add_to_waiting && !(*lb)->_in_waiting && !(*lb)->_in_rel)
            {
//                std::cerr << "\tWAITING!" << std::endl;
                (*lb)->_in_waiting = true;
                trans.push(lb->get());
            }
            return lb->get();
        };

                
        auto init = state_id(std::numeric_limits<size_t>::max(), std::numeric_limits<size_t>::max(), std::numeric_limits<uint32_t>::max());
        for (auto& rule : pda.states()[0]._rules) {
            assert(rule._operation == PDA::PUSH);
            edge_t* edge = insert_edge(rule._to, rule._op_label, init, true);
            trans.emplace(edge);
            { // insert bottom-element of stack
                auto e = insert_edge(init, std::numeric_limits<uint32_t>::max()-1, 0, false);
                e->_in_rel = true;
                rel.push_back(e);
            }
        }


        
        while (!trans.empty()) {
            auto t = trans.top();
            if (t->_to == 0)
                return true;
            trans.pop();
            if (t->_in_rel)
            {
                continue;
            }
            //if (!t->_in_rel)
            {
                t->_in_rel = true;
                rel.push_back(t);
            }
//            std::cerr << "(" << t->_from << ", " << t->_label << ", " << t->_to << ")" << std::endl;

            t->_in_waiting = false;
            if (t->_label != std::numeric_limits<uint32_t>::max()) {
                if(t->_from < pda.states().size())
                {
                    for (auto& rule : pda.states()[t->_from]._rules) {
                        assert(rule._to != 0);
                        if(!rule._precondition.contains(t->_label)) continue;
                        switch (rule._operation) {
                        case PDA::POP:
                            insert_edge(rule._to, std::numeric_limits<uint32_t>::max(), t->_to, true);
                            break;
                        case PDA::SWAP:
                            insert_edge(rule._to, rule._op_label, t->_to, true);
                            break;
                        case PDA::NOOP:
                            insert_edge(rule._to, t->_label, t->_to, true);
                            break;
                        case PDA::PUSH:
                        {
                            auto ns = state_id(t->_to, rule._to, rule._op_label);
                            insert_edge(rule._to, rule._op_label, ns, true);
                            auto re = insert_edge(ns, t->_label, t->_to, false);
                            if(re != nullptr && !re->_in_rel)
                            {
                                re->_in_rel = true;
                                rel.push_back(re);
                            }
                            for(auto e : rel)
                            {
                                if(e == nullptr) continue;
                                if(e->_to == ns && e->_label == std::numeric_limits<uint32_t>::max())
                                {
                                    insert_edge(e->_from, t->_label, t->_to, true);
                                }
                            }
                        }
                        break;
                        default:
                            assert(false);
                        }
                    }
                }
                else
                {
                    assert(false);
                }
            }
            else {
                // do some ordering/indexing on rel?
                for (auto o : rel) {
                    if(o == nullptr) continue;
                    if (o->_from == t->_to) {
                        insert_edge(t->_from, o->_label, o->_to, true);
                    }
                }
            }
        }
        return false;
    }

}
