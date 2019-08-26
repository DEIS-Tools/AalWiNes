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
 * File:   Moped.h
 * Author: Peter G. Jensen <root@petergjoel.dk>
 *
 * Created on July 24, 2019, 9:19 PM
 */

#ifndef MOPED_H
#define MOPED_H

#include "pdaaal/model/TypedPDA.h"

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
        bool verify(const TypedPDA<T>& pda, bool build_trace);

        template<typename T>        
        static void dump_pda(const TypedPDA<T>& pda, std::ostream& s);
        
        template<typename T>
        std::vector<typename TypedPDA<T>::tracestate_t> get_trace(TypedPDA<T>& pda) const;
        
    private:
        bool parse_result(const std::string&, bool build_trace);
        std::string _path;
        std::string _tmpfilepath;
        std::vector<std::string> _raw_trace;
        // trace?
    };

    template<typename T>
    bool Moped::verify(const TypedPDA<T>& pda, bool build_trace)
    {
        _raw_trace.clear();
        std::fstream file;
        file.open(_tmpfilepath, std::fstream::out);
        dump_pda(pda, file);
        file.close();
        return verify(_tmpfilepath, build_trace);
    }
    
    template<typename T>
    std::vector<typename TypedPDA<T>::tracestate_t> Moped::get_trace(TypedPDA<T>& pda) const
    {
        using tracestate_t = typename TypedPDA<T>::tracestate_t;
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
                            assert(*lit == 'l');
                            if(*lit != 'l')
                            {
                                throw base_error("Could not parse trace");
                            }
                            ++lit;
                            assert(fit > lit);
                            auto res = atoi(&*lit);
                            auto sym = pda.get_symbol(res);
                            trace.back()._stack.emplace_back(sym);                                
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
    void Moped::dump_pda(const TypedPDA<T>& pda, std::ostream& s) {
        using rule_t = typename TypedPDA<T>::rule_t;
        using state_t = typename TypedPDA<T>::state_t;
        auto write_op = [](std::ostream& s, const rule_t& rule, std::string noop) {
            assert(rule._to != 0);
            switch (rule._operation) {
                case TypedPDA<T>::SWAP:
                    s << "l" << rule._op_label;
                    break;
                case TypedPDA<T>::PUSH:
                    s << "l" << rule._op_label;
                    s << " ";
                case TypedPDA<T>::NOOP:
                    s << noop;
                    break;
                case TypedPDA<T>::POP:
                default:
                    break;
            }
        };

        s << "(I<_>)\n";
        // lets start by the initial transitions
        auto& is = pda.states()[0];
        for (auto& r : is._rules) {
            if (r._to != 0) {
                assert(r._operation == TypedPDA<T>::PUSH);
                s << "I<_> --> S" << r._to << "<";
                write_op(s, r, "_");
                s << ">\n";
            } else {
                assert(r._operation == TypedPDA<T>::NOOP);
                s << "I<_> --> D<_>\n";
                return;
            }
        }
        for (size_t sid = 1; sid < pda.states().size(); ++sid) {
            const state_t& state = pda.states()[sid];
            for (auto& r : state._rules) {
                if (r._to == 0) {
                    assert(r._operation == TypedPDA<T>::NOOP);
                    s << "S" << sid << "<_> --> DONE<_>\n";
                    continue;
                }
                if (r._precondition.empty()) continue;
                if(!r._precondition.wildcard())
                {
                    auto& symbols = r._precondition.labels();
                    for (auto& symbol : symbols) {
                        s << "S" << sid << "<l";
                        s << symbol;
                        s << "> --> ";
                        s << "S" << r._to;
                        s << "<";
                        std::stringstream ss;
                        ss << "l" << symbol;
                        write_op(s, r, ss.str());
                        s << ">\n";
                    }
                }
                else
                {
                    for (size_t symbol = 0; symbol < pda.number_of_labels(); ++symbol) {
                        s << "S" << sid << "<l";
                        s << symbol;
                        s << "> --> ";
                        s << "S" << r._to;
                        s << "<";
                        std::stringstream ss;
                        ss << "l" << symbol;
                        write_op(s, r, ss.str());
                        s << ">\n";
                    }                    
                }
            }
        }
    }
}

#endif /* MOPED_H */

