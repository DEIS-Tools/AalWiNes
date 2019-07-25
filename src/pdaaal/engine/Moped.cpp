/*
 *  Copyright Peter G. Jensen, all rights reserved.
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

    

}

