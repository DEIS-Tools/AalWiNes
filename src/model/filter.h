/*
 *  Copyright Peter G. Jensen, all rights reserved.
 */

/* 
 * File:   filter.h
 * Author: Peter G. Jensen <root@petergjoel.dk>
 *
 * Created on July 17, 2019, 5:52 PM
 */

#ifndef FILTER_H
#define FILTER_H

#include <unordered_set>
#include <functional>

namespace mpls2pda {
    class Network;
    struct filter_t {
        std::function<bool(const char*)> _from = [](const char*){return true;};
        std::function<bool(const char*, uint32_t, uint64_t, const char*, uint32_t, uint64_t, const char*)> _link = 
        [](const char*, uint32_t, uint64_t, const char*, uint32_t, uint64_t, const char*){return true;};
        filter_t operator&&(const filter_t& other);
    };
}

#endif /* FILTER_H */

