/*
 *  Copyright Peter G. Jensen, all rights reserved.
 */

#include "system.h"

#include <cstdio>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <array>

// Blatantly stolen from https://stackoverflow.com/questions/478898/how-do-i-execute-a-command-and-get-output-of-command-within-c-using-posix
#include <sstream>
std::string exec(const char* cmd) {
    std::array<char, 128> buffer;
    std::stringstream result;
    std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(cmd, "r"), pclose);
    if (!pipe) {
        throw std::runtime_error("popen() failed!");
    }
    while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
        result << buffer.data();
    }
    return result.str();
}


// Blatantly stolen from https://stackoverflow.com/questions/631664/accessing-environment-variables-in-c#631728

std::string get_env_var(std::string const & key)
{
    return get_env_var(key.c_str());
}

std::string get_env_var(const char* key)
{
    char * val = getenv(key);
    return val == nullptr ? std::string("") : std::string(val);
}
