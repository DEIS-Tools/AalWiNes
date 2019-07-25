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
            e << "An error occured during verification using moped\n\n" << result;
            throw std::runtime_error(e.str());
        }
        _trace.clear();
        if(build_trace && res)
        {
            
        }
        return res;
    }

    

}

