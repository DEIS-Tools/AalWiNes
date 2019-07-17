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