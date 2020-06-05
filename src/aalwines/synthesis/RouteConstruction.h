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
 * File:   RouteConstruction.h
 * Author: Morten K. Schou <morten@h-schou.dk>
 *
 * Created on 01-04-2020
 */

#ifndef AALWINES_ROUTECONSTRUCTION_H
#define AALWINES_ROUTECONSTRUCTION_H

#include <aalwines/model/Network.h>

namespace aalwines {
    class RouteConstruction {
        using label_t = RoutingTable::label_t;

    public:
        static bool make_reroute(const Interface* failed_inf, const std::function<label_t(void)>& next_label,
                const std::function<uint32_t(const Interface*)>& cost_fn = [](const Interface* interface){return 1;});

        static bool make_reroute(const Interface* failed_inf, const std::function<label_t(void)>& next_label,
                                 const std::unordered_map<const Interface*,uint32_t>& cost_map) {
            return RouteConstruction::make_reroute(failed_inf, next_label, [&cost_map](const Interface* interface) {
                return cost_map.at(interface);
            });
        }

        static bool make_data_flow(const Interface* from, const Interface* to,
                const std::function<label_t(Query::type_t)>& next_label, const std::vector<const Router*>& path);
        static bool make_data_flow(Interface* from, const std::vector<Interface*>& path,
                const std::function<label_t(Query::type_t)>& next_label);
        static bool make_data_flow(Interface* from, Interface* to, const std::function<label_t(Query::type_t)>& next_label,
                const std::function<uint32_t(const Interface*)>& cost_fn = [](const Interface* interface){return 1;});
        static bool make_data_flow(Interface* from, Interface* to, const std::function<label_t(Query::type_t)>& next_label,
                const std::unordered_map<const Interface*,uint32_t>& cost_map) {
            return RouteConstruction::make_data_flow(from, to, next_label, [&cost_map](const Interface* interface) {
                return cost_map.at(interface);
            });
        }
    };
}

#endif //AALWINES_ROUTECONSTRUCTION_H
