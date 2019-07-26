/*
 *  Copyright Peter G. Jensen, all rights reserved.
 */

/* 
 * File:   Moped.h
 * Author: Peter G. Jensen <root@petergjoel.dk>
 *
 * Created on July 24, 2019, 9:19 PM
 */

#ifndef MOPED_H
#define MOPED_H

#include "pdaaal/model/PDA.h"

#include <cassert>
#include <sstream>
#include <fstream>
#include <functional>
#include <vector>

namespace pdaaal {

    class Moped {
    public:
        Moped();
        ~Moped();
        bool verify(const std::string& tmpfile, bool build_trace);
        
        template<typename T>
        bool verify(const PDA<T>& pda, bool build_trace, std::function<void(std::ostream&, const T&) > labelprinter);
        
        template<typename T>
        static void dump_pda(const PDA<T>& pda, std::ostream& s, std::function<void(std::ostream&, const T&) > labelprinter);
        
        template<typename T>
        std::vector<typename PDA<T>::tracestate_t> get_trace(std::function<T(const char*, const char*)> label_reader) const;
        
    private:
        bool parse_result(const std::string&, bool build_trace);
        std::string _path;
        std::string _tmpfilepath;
        std::vector<std::string> _raw_trace;
        // trace?
    };

    template<typename T>
    bool Moped::verify(const PDA<T>& pda, bool build_trace, std::function<void(std::ostream&, const T&) > labelprinter)
    {
        std::fstream file;
        file.open(_tmpfilepath, std::fstream::out);
        dump_pda(pda, file, labelprinter);
        file.close();
        return verify(_tmpfilepath, build_trace);
    }
    
    template<typename T>
    std::vector<typename PDA<T>::tracestate_t> Moped::get_trace(std::function<T(const char*, const char*)> label_reader) const
    {
        using tracestate_t = typename PDA<T>::tracestate_t;
        std::vector<tracestate_t> trace;
        std::stringstream error;
        for(size_t sid = 0; sid < _raw_trace.size(); ++sid)
        {
            auto& s = _raw_trace[sid];
            if(s[0] != 'S')
            {
                error << "Could not find state in trace-line : " << s;
                throw base_error(error.str());                
            }
            const char* fit = s.c_str();
            ++fit; // skip 'S'
            const char* lit = fit;
            trace.emplace_back();
            trace.back()._pdastate = std::strtoll(fit, (char**)&lit, 10);
            fit = lit;
            
            while(*fit != '<' && *fit != '\n') ++fit;
            if(*fit != '<')
            {
                error << "Expected '<' to start stack after state: " << s;
                throw base_error(error.str());     
            }
            ++fit;
            while(true)
            {
                if(*fit != '\n')
                {
                    lit = fit;
                    while(*fit != '\0' && *fit != ' ' && *fit != '_') ++fit;
                    if(*fit == '_')
                    {
                        if(*(fit+1) != '>')
                        {
                            error << "Unexpected end of stack, expected '_>' : " << s;
                            throw base_error(error.str());
                        }
                        else if(*(fit-1) == ' ')
                        {
                            // just the end of this stack!
                            break;
                        }
                        else if(sid != (_raw_trace.size() - 1))
                        {
                            error << "Unexpected end of trace at line " << sid << " : " << s << std::endl;
                            error << "Still on stack : \n";
                            for(++sid; sid < _raw_trace.size(); ++sid)
                                error << "\t\"" << _raw_trace[sid] << "\"\n";
                            throw base_error(error.str());                        
                        }
                        else
                        {
                            // end of trace (and stack)
                            break;
                        }
                    }
                    else if(*fit == ' ')
                    {
                        if(lit == fit)
                        {
                            // just whitespace, skip!
                            ++fit;
                            ++lit;
                        }
                        else
                        {
                            assert(fit > lit);
                            if((fit - lit) == 3 && strncmp(lit, "DOT", 3) == 0)
                            {
                                trace.back()._stack.emplace_back(true, 0);
                            }
                            else
                            {
                                trace.back()._stack.emplace_back(false, label_reader(lit, fit));                                
                            }
                        }
                        continue;
                    }
                    else if(*fit == '\0')
                    {
                        error << "Unexpected end of line: " << s;
                        throw base_error(error.str());
                    }
                }
                else
                {
                    error << "Unexpected end of line: " << s;
                    throw base_error(error.str());                                    
                }
            }
        }
        return trace;
    }
    
    template<typename T>
    void Moped::dump_pda(const PDA<T>& pda, std::ostream& s, std::function<void(std::ostream&, const T&) > labelprinter) {
        using rule_t = typename PDA<T>::rule_t;
        using state_t = typename PDA<T>::state_t;
        auto write_op = [&labelprinter](std::ostream& s, const rule_t& rule, std::string noop) {
            assert(rule._to != 0);
            switch (rule._operation) {
                case PDA<T>::SWAP:
                    if (rule._dot_label)
                        s << "DOT";
                    else {
                        labelprinter(s, rule._op_label);
                    }
                    break;
                case PDA<T>::PUSH:
                    if (rule._dot_label)
                        s << "DOT ";
                    else {
                        labelprinter(s, rule._op_label);
                        s << " ";
                    }
                case PDA<T>::NOOP:
                    s << noop;
                    break;
                case PDA<T>::POP:
                default:
                    break;
            }
        };

        s << "(I<_>)\n";
        // lets start by the initial transitions
        auto& is = pda.states()[0];
        for (auto& r : is._rules) {
            if (r._to != 0) {
                assert(r._operation == PDA<T>::PUSH);
                s << "I<_> --> S" << r._to << "<";
                write_op(s, r, "_");
                s << ">\n";
            } else {
                assert(r._op_label == PDA<T>::NOOP);
                s << "I<_> --> D<_>\n";
                return;
            }
        }
        for (size_t sid = 1; sid < pda.states().size(); ++sid) {
            const state_t& state = pda.states()[sid];
            for (auto& r : state._rules) {
                if (r._to == 0) {
                    assert(r._operation == PDA<T>::NOOP);
                    s << "S" << sid << "<_> --> DONE<_>\n";
                    continue;
                }
                if (r._precondition.empty()) continue;
                if (state._pre_is_dot && (r._precondition._wildcard || r._operation == PDA<T>::POP || r._operation == PDA<T>::SWAP)) {
                    // we can make a DOT -> DOT rule but only if both pre and post are "DOT" -- 
                    // or as the case with POP and SWAP; TOS can be anything 
                    s << "S" << sid << "<DOT> --> ";
                    s << "S" << r._to;
                    s << "<";
                    write_op(s, r, "DOT");
                    s << ">\n";
                } else if (state._pre_is_dot) {
                    // we effectively settle the state of the DOT symbol here
                    assert(!r._precondition._wildcard);
                    for (auto& symbol : r._precondition._labels) {
                        std::stringstream ss;
                        labelprinter(ss, symbol);
                        s << "S" << sid << "<DOT> --> ";
                        s << "S" << r._to;
                        s << "<";
                        write_op(s, r, ss.str());
                        s << ">\n";
                    }
                } else {
                    assert(!state._pre_is_dot);
                    bool post_settle = false;
                    auto& symbols = r._precondition._wildcard ? pda.labelset() : r._precondition._labels;
                    for (auto& symbol : symbols) {
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
                        if (r._precondition._wildcard && (r._operation == PDA<T>::NOOP || r._operation == PDA<T>::POP || r._operation == PDA<T>::PUSH)) {
                            // we do not need to settle the DOT yet
                            post_settle = true;
                        } else {
                            // settle the DOT
                            s << "S" << sid << "<DOT> " << postss.str();
                        }
                    }
                    if (post_settle) {
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

}

#endif /* MOPED_H */

