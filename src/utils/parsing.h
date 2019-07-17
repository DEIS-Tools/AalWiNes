/*
 *  Copyright Peter G. Jensen, all rights reserved.
 */

/* 
 * File:   parsing.h
 * Author: Peter G. Jensen <root@petergjoel.dk>
 *
 * Created on July 17, 2019, 4:19 PM
 */

#ifndef PARSING_H
#define PARSING_H

#include <ostream>
#include <inttypes.h>
#include <limits>

void write_ip4(std::ostream& s, uint32_t ip);

uint32_t parse_ip4(const char* s);

#endif /* PARSING_H */

