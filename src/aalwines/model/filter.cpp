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

#include "filter.h"
#include "Network.h"

namespace aalwines {

    filter_t filter_t::operator&&(const filter_t& other) {
        filter_t ret;
        ret._from = [tf=_from,of=other._from](const std::string& f) {
            return tf(f) && of(f);
        };
        ret._link = [tl=_link,ol=other._link](const std::string& fn, const std::string& tn, const std::string& tr) {
            return tl(fn, tn, tr) && ol(fn, tn, tr);
        };
        return ret;
    }

}