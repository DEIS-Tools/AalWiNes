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
 *  Copyright Dan Kristiansen, Morten K. Schou
 */

/*
 * File:   NetworkWeight.h
 * Author: Dan Kristiansen & Morten K. Schou
 *
 * Created on March 16, 2020
 */

#ifndef AALWINES_NETWORKWEIGHT_H
#define AALWINES_NETWORKWEIGHT_H

#include <pdaaal/Weight.h>

namespace aalwines {


    using weight_function = std::function<uint32_t(RoutingTable::forward_t, bool)>;

    static weight_function default_weight_function = [](RoutingTable::forward_t r, bool _) -> uint32_t {
        return 0;
    };

    static weight_function stack_size = [](RoutingTable::forward_t r, bool _) -> uint32_t {
        return std::count_if(r._ops.begin(), r._ops.end(), [](RoutingTable::action_t act) -> bool { return act._op == RoutingTable::op_t::PUSH; });
    };

    static weight_function latency = [](RoutingTable::forward_t r, bool _) -> uint32_t {
        return 0;
    };

    static weight_function num_hops = [](RoutingTable::forward_t r, bool last_op) -> uint32_t {
        return last_op && !r._via->is_virtual() ? 1 : 0;
    };

    static weight_function link_failures = [](RoutingTable::forward_t r, bool _) -> uint32_t {
        return r._weight;
    };
}

#endif //AALWINES_NETWORKWEIGHT_H
