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
 * File:   Query.cpp
 * Author: Peter G. Jensen <root@petergjoel.dk>
 * 
 * Created on July 16, 2019, 5:38 PM
 */

#include "Query.h"


namespace mpls2pda {

    Query::Query(pdaaal::NFA<label_t>&& pre, pdaaal::NFA<label_t>&& path, pdaaal::NFA<label_t>&& post, int lf, mode_t mode)
    : _prestack(std::move(pre)), _poststack(std::move(post)), _path(std::move(path)), _link_failures(lf), _mode(mode)
    {

    }

}