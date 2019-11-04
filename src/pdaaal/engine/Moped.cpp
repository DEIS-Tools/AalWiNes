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
 * File:   Moped.cpp
 * Author: Peter G. Jensen <root@petergjoel.dk>
 * 
 * Created on July 24, 2019, 9:19 PM
 */

#include "Moped.h"
#include "utils/system.h"

#include <boost/filesystem.hpp>
#include <fstream>
#include <iostream>

namespace pdaaal
{

    Moped::Moped()
    {
        _tmpfilepath = boost::filesystem::unique_path().native();    
        std::fstream file;
        file.open(_tmpfilepath, std::fstream::out);
        file.close();
    }

    Moped::~Moped()
    {
        boost::filesystem::remove(_tmpfilepath);
    }

    bool Moped::verify(const std::string& tmpfile, bool build_trace)
    {
        // system call here!
        auto moped = get_env_var("MOPED_PATH");
        if(moped.empty())
        {
            throw base_error("MOPED_PATH environment variable not set");
        }
        std::stringstream cmd;
        cmd << moped;
        if(build_trace)
            cmd << " -t";
        cmd << " -s0 " << tmpfile;
        cmd << " -r DONE:_";
        auto cstr = cmd.str();
        auto res = exec(cstr.c_str());
        return parse_result(res, build_trace);
    }
    
    bool Moped::parse_result(const std::string& result, bool build_trace)
    {
        bool res = false;
        if(result.substr(0, 4) == "YES.")
        {
            res = true;
        }
        else if(result.substr(0, 3) == "NO.")
        {
            res = false;
        }
        else
        {
            std::stringstream e;
            e << "An error occurred during verification using moped\n\n" << result;
            throw base_error(e.str());
        }
        _raw_trace.clear();
        if(build_trace && res)
        {
            std::istringstream is(result);
            bool saw_start = false;
            bool saw_init = false;
            bool saw_end = false;
            std::string buffer;
            while(std::getline(is, buffer))
            {
                if(buffer.empty()) continue;
                if(saw_start)
                {
                    if(buffer[0] == 'I' && !saw_init && !saw_end)
                    {
                        saw_init = true;
                    }
                    else if(buffer[0] == 'D' && saw_init && !saw_end)
                    {
                        saw_end = true;
                    }
                    else if(buffer[0] == 'S' && saw_init && !saw_end)
                    {
                        _raw_trace.push_back(buffer);
                    }
                    else if(buffer.substr(2, 18) == "[ target reached ]" && saw_init && saw_end)
                    {
                        break;
                    }
                    else
                    {
                        std::stringstream e;
                        e << "An error occurred during parsing of trace from moped\n\n";
                        if(saw_init) e << "\"I <_>\" seen\n";
                        if(saw_end) e << "\"DONE <_>\" seen\n";
                        e << buffer << "\n\n" << result;
                        throw base_error(e.str());                        
                    }
                }
                else if(buffer.substr(0, 13) == "--- START ---")
                {
                    saw_start = true;
                }
            }
            if(!saw_end)
            {
                std::stringstream e;
                e << "Missing accepting state in trace from Moped\n\n" << result;
                throw base_error(e.str());                                        
            }
        }
        return res;
    }

    bool Moped::verify(const PDA& pda, bool build_trace)
    {
        _raw_trace.clear();
        std::fstream file;
        file.open(_tmpfilepath, std::fstream::out | std::fstream::trunc);
        dump_pda(pda, file);
        file.close();
        return verify(_tmpfilepath, build_trace);
    }

    void Moped::dump_pda(const PDA& pda, std::ostream& s) {
        using rule_t = PDA::rule_t;
        using state_t = PDA::state_t;
        auto write_op = [](std::ostream& s, const rule_t& rule, std::string noop) {
            assert(rule._to != 0);
            switch (rule._operation) {
                case PDA::SWAP:
                    s << "l" << rule._op_label;
                    break;
                case PDA::PUSH:
                    s << "l" << rule._op_label;
                    s << " ";
                case PDA::NOOP:
                    s << noop;
                    break;
                case PDA::POP:
                default:
                    break;
            }
        };

        s << "(I<_>)\n";
        // lets start by the initial transitions
        auto& is = pda.states()[0];
        for (auto& r : is._rules) {
            if (r._to != 0) {
                assert(r._operation == PDA::PUSH);
                s << "I<_> --> S" << r._to << "<";
                write_op(s, r, "_");
                s << ">\n";
            } else {
                assert(r._operation == PDA::NOOP);
                s << "I<_> --> D<_>\n";
                return;
            }
        }
        for (size_t sid = 1; sid < pda.states().size(); ++sid) {
            const state_t& state = pda.states()[sid];
            for (auto& r : state._rules) {
                if (r._to == 0) {
                    assert(r._operation == PDA::NOOP);
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

