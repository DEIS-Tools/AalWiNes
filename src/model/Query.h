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
 * File:   Query.h
 * Author: Peter G. Jensen <root@petergjoel.dk>
 *
 * Created on July 16, 2019, 5:38 PM
 */

#ifndef QUERY_H
#define QUERY_H
#include "pdaaal/model/NFA.h"
#include "Router.h"

#include <ostream>

namespace mpls2pda {
class Query {
public:
    enum mode_t {
        OVER, UNDER, EXACT
    };
    using label_t = ssize_t;
    Query() {};
    Query(pdaaal::NFA<label_t>&& pre, pdaaal::NFA<label_t>&& path, pdaaal::NFA<label_t>&& post, int lf, mode_t mode);
    Query(Query&&) = default;
    Query(const Query&) = default;
    virtual ~Query() = default;
    Query& operator=(Query&&) = default;
    Query& operator=(const Query&) = default;
    
    void print_dot(std::ostream& out);
private:
    pdaaal::NFA<label_t> _prestack;
    pdaaal::NFA<label_t> _poststack;
    pdaaal::NFA<label_t> _path;
    int _link_failures = 0;
    mode_t _mode;

};
}
#endif /* QUERY_H */

