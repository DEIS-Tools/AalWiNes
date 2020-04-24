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
 * File:   coordinate.cpp
 * Author: Morten K. Schou <morten@h-schou.dk>
 *
 * Created on 06-04-2020
 */

#include "coordinate.h"
#include <cmath>

namespace aalwines {

    Coordinate::Coordinate(double latitude, double longitude) : _latitude(latitude), _longitude(longitude),
        _rad_lat(M_PI / 180 * latitude), _rad_long(M_PI / 180 * longitude) {}

    double Coordinate::distance_to(const Coordinate& other) const {
        // Using the Haversine formula.
        return _R_km * 2 * std::asin(std::sqrt(std::pow((_rad_lat - other._rad_lat)/2, 2) +
        std::cos(_rad_lat) * std::cos(other._rad_lat) * std::pow(std::sin((_rad_long - other._rad_long) / 2), 2)));
    }

    void Coordinate::write_xml_attributes(std::ostream& s) const {
        s << " latitude=\"" << _latitude << "\" longitude=\"" << _longitude << "\" ";
    }
}

