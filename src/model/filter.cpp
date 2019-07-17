/*
 *  Copyright Peter G. Jensen, all rights reserved.
 */

#include "filter.h"
#include "Network.h"

namespace mpls2pda {

    filter_t filter_t::operator&&(const filter_t& other)
    {
        filter_t ret;
        auto tf = _from;
        auto of = other._from;
        ret._from = [of,tf](const char* f)
        {
            return tf(f) && 
                   of(f);
        };
        auto ol = other._link;
        auto tl = _link;
        ret._link = [tl,ol](const char* fn, uint32_t fi4, uint64_t fi6, const char* tn, uint32_t ti4, uint64_t ti6, const char* tr)
        {
            return tl(fn, fi4, fi6, tn, ti4, ti6, tr) && 
                   ol(fn, fi4, fi6, tn, ti4, ti6, tr);
        };
        return ret;
    }

}