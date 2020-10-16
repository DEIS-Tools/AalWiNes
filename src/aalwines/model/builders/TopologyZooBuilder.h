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
 *  Copyright Dan Kristiansen
 */

/*
 * File:   TopologyZooBuilder.h
 * Author: Dan Kristiansen
 *
 * Created on 04-06-2020.
 */

#ifndef AALWINES_TOPOLOGYZOOBUILDER_H
#define AALWINES_TOPOLOGYZOOBUILDER_H

#include <aalwines/model/Network.h>

#include <utility>

namespace aalwines {
    class TopologyZooBuilder {
    public:
        static Network parse(const std::string& gml);
    };
}


#endif //AALWINES_TOPOLOGYZOOBUILDER_H