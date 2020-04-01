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
 *  Copyright Morten K. Schou
 */

/*
 * File:   FastRerouting.h
 * Author: Morten K. Schou <morten@h-schou.dk>
 *
 * Created on 01-04-2020
 */

#ifndef AALWINES_FASTREROUTING_H
#define AALWINES_FASTREROUTING_H

#include <aalwines/model/Network.h>

namespace aalwines {
    class FastRerouting {
        using label_t = RoutingTable::label_t;

    public:
        static bool make_reroute(Network &network, const Interface* failed_inf, label_t failover_label);

    };
}

#endif //AALWINES_FASTREROUTING_H
