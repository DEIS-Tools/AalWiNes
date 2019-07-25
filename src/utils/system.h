/*
 *  Copyright Peter G. Jensen, all rights reserved.
 */

/* 
 * File:   system.h
 * Author: Peter G. Jensen <root@petergjoel.dk>
 *
 * Created on July 24, 2019, 10:42 PM
 */

#include <string>

#ifndef SYSTEM_H
#define SYSTEM_H

std::string exec(const char* cmd);
std::string get_env_var( std::string const & key );
std::string get_env_var( const char* key );

#endif /* SYSTEM_H */

