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
 * File:   PDA.h
 * Author: Peter G. Jensen <root@petergjoel.dk>
 *
 * Created on July 18, 2019, 3:24 PM
 */

#ifndef PDA_H
#define PDA_H

#include <string>
#include <sstream>
#include <vector>
#include <ostream>
#include <unordered_set>

#include "NFA.h"

namespace pdaaal {

    template<typename T>
    class PDA {
    public:
        enum op_t {PUSH, POP, SWAP, NOOP};
        struct rule_t
        {
            op_t _op = POP;
            T _pre;
            size_t _dest;
            T _op_label;
        };
        
        using nfastate_t = typename NFA<T>::state_t;
    public:
        PDA(NFA<T>& prestack, NFA<T>& poststack, std::unordered_set<T>&& all_labels)
        : _cons_stack(prestack), _des_stack(poststack), _all_labels(all_labels)
        {
            _cons_stack.compile();
            _des_stack.compile();
        };
        
        protected:
        virtual const std::vector<size_t>& initial() = 0;
        virtual bool empty_accept() const = 0; 
        virtual bool accepting(size_t)  = 0;
        virtual std::vector<rule_t> rules(size_t ) = 0;
        
        public:
        std::string print_moped(std::ostream& out, 
                std::function<void(std::ostream&,const T&)> printer = [](auto& o, auto&e) { o << e; })
        {
            // CONSTRUCTION NFA
            out << "(I<_>)\n";
            bool has_empty_accept = false;
            std::unordered_set<const nfastate_t*> seen;
            std::vector<const nfastate_t*> waiting;
            // INITIAL (step to next of initial, not initial)
            for(auto& i : _cons_stack.initial())
            {
                // We do the first step here.
                if(i->_accepting)
                {
                    has_empty_accept = true;
                }
                for(auto& e : i->_edges)
                {
                    std::vector<nfastate_t*> next{e._destination};
                    NFA<T>::follow_epsilon(next);            
                    bool some = false;
                    auto foreach = [&,this](auto& s) {
                        bool ok = !e._negated;
                        if(e._negated)
                        {
                            ok = true;
                            auto lb = std::lower_bound(e._symbols.begin(), e._symbols.end(), s);
                            if(lb != std::end(e._symbols) && *lb == s)
                                ok = false;
                        }
                        if(ok)
                        {
                            for(auto& n : next)
                            {
                                some = true;
                                out << "I<_> --> C" << n << "<";
                                printer(out, s);
                                out << " _>\n";
                            }
                        }
                    };
                    // special case for WILDCARD opt
                    if(e.wildcard(_all_labels.size()))
                    {
                        some = true;
                        for(auto n : next)
                            out << "I<_> --> C" << n << "<DOT _>\n";
                    }
                    else if(e._negated) for(auto& s : _all_labels) foreach(s);
                    else                for(auto& s : e._symbols)  foreach(s);
                    if(some) for(auto n : next)
                    {
                        if(seen.count(n) == 0)
                        {
                            seen.insert(n);
                            waiting.push_back(n);
                        }
                    }
                }                
            }
            
            
            // REST
            while(!waiting.empty())
            {
                auto top = waiting.back();
                waiting.pop_back();
                // we need to know the set of pre-edges to be able to handle the TOS correctly
                auto pre = top->prelabels(_all_labels);
                
                for(auto& e : top->_edges)
                {
                    std::vector<nfastate_t*> next{e._destination};
                    NFA<T>::follow_epsilon(next);
                    bool some = false;
                    auto foreach = [&,this](auto& s) {
                        bool ok = !e._negated;
                        if(e._negated)
                        {
                            ok = true;
                            auto lb = std::lower_bound(e._symbols.begin(), e._symbols.end(), s);
                            if(lb != std::end(e._symbols) && *lb == s)
                                ok = false;
                        }
                        if(ok)
                        {
                            for(auto& n : next)
                            {
                                if(pre.size() == _all_labels.size())
                                {
                                    out << "C" << top << "<DOT> --> C" << n << "<";
                                    printer(out, s);
                                    out << " DOT>\n";
                                    some = true;
                                }
                                else
                                {
                                    for(auto& ss : pre)
                                    {
                                        out << "C" << top << "<";
                                        printer(out, ss);
                                        out<< "> --> C" << n << "<";
                                        printer(out, s);
                                        out << " ";
                                        printer(out, ss);
                                        out << ">\n";                            
                                        some = true;
                                    }
                                }
                            }
                        }
                    };
                    
                    if(e.wildcard(_all_labels.size()))
                    {
                        if(pre.size() == _all_labels.size())
                        {
                            for(auto n : next)
                            {
                                out << "C" << top << "<DOT> --> C" << n << "<DOT DOT>\n"; 
                                some = true;
                            }
                        }
                        else
                        {
                            for(auto n : next)
                            {
                                for(auto& p : pre)
                                {
                                    some = true;
                                    out << "C" << top << "<";
                                    printer(out, p);
                                    out << "> --> C" << n << "<DOT ";
                                    printer(out, p);
                                    out << ">\n";
                                }
                            }
                        }
                    }
                    else if(e._negated) for(auto& s : _all_labels) foreach(s);
                    else                for(auto& s : e._symbols) foreach(s);
                    if(some) for(auto n : next)
                    {
                        if(seen.count(n) == 0)
                        {
                            seen.insert(n);
                            waiting.push_back(n);
                        }
                    }
                }
                
                // CONNECT TO NEXT
                if(top->_accepting)
                {
                    // TODO there is some nice and easy pruning to be done here
                    // by just considering "next step".
                    auto to_next = [&,this] (auto& nodes, char M)
                    {
                        for(auto& sd : nodes)
                        {
                            // TODO:
                            // we can prune a lot here by checking if elements
                            // from pre are accepted by the next state
                            if(pre.size() == _all_labels.size())
                            {
                                out << "C" << top << "<DOT> --> " << M << sd << "<DOT>\n";
                            }
                            else
                            {
                                for(auto& p : pre)
                                {
                                    // TODO, match with outgoing rules of initial here
                                    out << "C" << top << "<";
                                    printer(out, p);
                                    out << "> --> " << M << sd << "<";
                                    printer(out, p);
                                    out << ">\n";
                                }
                            }
                        }
                    };
                    to_next(initial(), 'P');
                    if(empty_accept())
                        to_next(_des_stack.initial(), 'D');
                }
            }

            bool des_empty_accept = false;
            for(auto s : _des_stack.initial())
            {
                if(s->_accepting)
                {
                    des_empty_accept = true;
                    break;
                }
            }

            // PATH & NETWORK
            // SYNC!
            waiting.clear();
            seen.clear();
            auto pdawaiting = initial();
            std::unordered_set<size_t> pdaseen(pdawaiting.begin(), pdawaiting.end());
            while(!pdawaiting.empty())
            {
                auto top = pdawaiting.back();
                pdawaiting.pop_back();
                if(accepting(top))
                {
                    // any label could really be on the top of the stack here
                    // do first step of DES
                    if(des_empty_accept)
                    {
                        out << "P" << top << "<_> --> DONE<_>\n";
                    }
                    for(auto ds : _des_stack.initial())
                    {
                        for(auto& e : ds->_edges)
                        {
                            if(!e.empty(_all_labels.size()))
                            {
                                out << "P" << top << "<DOT> --> D" << e._destination << "<>\n";
                            }
                            std::vector<nfastate_t*> next{e._destination};
                            NFA<T>::follow_epsilon(next);
                            if(!e._negated)
                            {
                                for(auto& s : e._symbols)
                                {
                                    for(auto n : next)
                                    {
                                        out << "P" << top << "<";
                                        printer(out, s);
                                        out << "> --> D" << n << "<>\n";
                                    }
                                }
                            }
                            else 
                            {
                                for(auto& s : _all_labels)
                                {
                                    auto lb = std::lower_bound(e._symbols.begin(), e._symbols.end(), s);
                                    if(lb != std::end(e._symbols) && *lb != s)
                                    {
                                        for(auto n : next)
                                        {
                                            out << "P" << top << "<";
                                            printer(out, s);
                                            out << "> --> D" << n << "<>\n";
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
                for(auto& r : rules(top))
                {
                    out << "P" << top << "<";
                    printer(out, r._pre);
                    out << "> --> ";
                    out << "P" << r._dest << "<";
                    if(r._op == PUSH || r._op == SWAP)
                        printer(out, r._op_label);
                    if(r._op == PUSH)
                    {
                        out << " ";
                        printer(out, r._pre);
                    }
                    if(r._op == NOOP)
                        printer(out, r._pre);
                    out << ">\n";
                    out << "P" << top << "<DOT> --> ";
                    out << "P" << r._dest << "<";
                    if(r._op == PUSH || r._op == SWAP)
                        printer(out, r._op_label);
                    if(r._op == PUSH)
                    {
                        out << " ";
                        printer(out, r._pre);
                    }
                    if(r._op == NOOP)
                        out << "DOT";
                    out << ">\n";
                    if(pdaseen.count(r._dest) == 0)
                    {
                        pdaseen.insert(r._dest);
                        pdawaiting.push_back(r._dest);
                    }
                }
            }
            
            
            // DESTRUCTION NFA
            if(has_empty_accept && empty_accept() && des_empty_accept)
            {
                out << "I<_> --> DONE<_> \"empty\"\n";
            }          

            std::vector<nfastate_t*> waiting_next = _des_stack.initial();
            std::unordered_set<nfastate_t*> seen_next(waiting_next.begin(), waiting_next.end());
            
            while(!waiting_next.empty())
            {
                auto top = waiting_next.back();
                waiting_next.pop_back();
                if(top->_accepting)
                {
                    out << "D" << top << "<_> --> DONE<_>\n";
                }
                for(auto& e : top->_edges)
                {
                    std::vector<nfastate_t*> next{e._destination};
                    NFA<T>::follow_epsilon(next);
                    bool some = false;

                    auto foreach = [&](auto& s)
                    {
                        bool ok = !e._negated;
                        if(e._negated)
                        {
                            ok = true;
                            auto lb = std::lower_bound(e._symbols.begin(), e._symbols.end(), s);
                            if(lb != std::end(e._symbols) && *lb == s)
                                ok = false;
                        }
                        if(ok)
                        {
                            for(auto& n : next)
                            {
                                out << "D" << top << "<";
                                printer(out, s);
                                out << "> --> D" << n << "<>\n";
                                some = true;
                            }
                        }
                    };
                    if(e._negated) for(auto& s : _all_labels) foreach(s);
                    else           for(auto& s : e._symbols) foreach(s);
                    if(some)
                    {
                        out << "D" << top << "<DOT> --> D" << e._destination << "<>\n";
                        for(auto& s : next)
                        {
                            if(seen_next.count(s) == 0)
                            {
                                waiting_next.push_back(s);
                                seen_next.insert(s);
                            }
                        }
                    }
                }
            }
            
            /*            
            
            
            out << "(s" << initial() << "<_>)\n";
            for(auto s : states())
            {
                auto& state = (*this)[s];
                
                for(auto& r : rules(s))
                {
                    for(auto& p : (r._any ? all_labels() : r._pre))
                    {
                        out << "s" << s << "<l";
                        printer(out, p);
                        out << "> --> s" << r._dest << "<";
                        if(r._op == SWAP)
                        {
                            printer(out, r._op_label);
                        }
                        if(r._op == PUSH)
                        {
                            printer(out, r._op_label);
                            printer(out, p);
                        }
                        out << ">";
                        tracer(out, state, r, p);
                        out << "\n";
                    }
                    if(s == initial() && r._op == PUSH)
                    {
                        out << "s" << s << "<_> --> s" << r._dest << "<" << printer(out, r._op_label); << ">\n";
                    }
                }
            }
            std::stringstream ss;
            ss << "s" << initial() << ":s" << accepting();
            return ss.str();*/
            return "";
        }        
    protected:
        NFA<T> _cons_stack, _des_stack;
        std::unordered_set<T> _all_labels;
    private:

    };
}

#endif /* PDA_H */

