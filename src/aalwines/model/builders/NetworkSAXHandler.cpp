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
 * File:   NetworkSAXHandler.cpp
 * Author: Morten K. Schou <morten@h-schou.dk>
 *
 * Created on 19-10-2020.
 */

#include "NetworkSAXHandler.h"

namespace aalwines {


    std::ostream& operator<<(std::ostream& s, aalwines::NetworkSAXHandler::keys key) {
        switch (key) {
            case NetworkSAXHandler::keys::none:
                break;
            case NetworkSAXHandler::keys::network:
                s << "network";
                break;
            case NetworkSAXHandler::keys::network_name:
                s << "name";
                break;
            case NetworkSAXHandler::keys::routers:
                s << "routers";
                break;
            case NetworkSAXHandler::keys::links:
                s << "links";
                break;
            case NetworkSAXHandler::keys::router_names:
                s << "names";
                break;
            case NetworkSAXHandler::keys::location:
                s << "location";
                break;
            case NetworkSAXHandler::keys::latitude:
                s << "latitude";
                break;
            case NetworkSAXHandler::keys::longitude:
                s << "longitude";
                break;
            case NetworkSAXHandler::keys::interfaces:
                s << "interfaces";
                break;
            case NetworkSAXHandler::keys::interface_name:
                s << "name";
                break;
            case NetworkSAXHandler::keys::routing_table:
                s << "routing_table";
                break;
            case NetworkSAXHandler::keys::from_router:
                s << "from_router";
                break;
            case NetworkSAXHandler::keys::from_interface:
                s << "from_interface";
                break;
            case NetworkSAXHandler::keys::to_router:
                s << "to_router";
                break;
            case NetworkSAXHandler::keys::to_interface:
                s << "to_interface";
                break;
            case NetworkSAXHandler::keys::bidirectional:
                s << "bidirectional";
                break;
        }
        return s;
    }
}