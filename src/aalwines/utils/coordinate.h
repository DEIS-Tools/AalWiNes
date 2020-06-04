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
 * File:   coordinate.h
 * Author: Morten K. Schou <morten@h-schou.dk>
 *
 * Created on 06-04-2020
 */

#ifndef AALWINES_COORDINATE_H
#define AALWINES_COORDINATE_H

#include <ostream>
#include <vector>

namespace aalwines {

    class Coordinate {
    public:
        explicit Coordinate(std::pair<double, double> pair) : Coordinate(pair.first, pair.second) {};
        Coordinate(double latitude, double longitude);
        [[nodiscard]] double distance_to(const Coordinate& other) const;
        [[nodiscard]] double latitude() const { return _latitude; }
        [[nodiscard]] double longitude() const { return _longitude; }
        void write_xml_attributes(std::ostream& s) const;
        static Coordinate mid_point(const std::vector<Coordinate>& coordinates);
        bool operator==(const Coordinate& other) const {
            return _latitude == other._latitude && _longitude == other._longitude;
        }
        bool operator!=(const Coordinate& other) const {
            return !(*this == other);
        }

    private:
        static constexpr double _R_km = 6372.8;
        const double _latitude;
        const double _longitude;
        const double _rad_lat;
        const double _rad_long;
    };
}

#endif //AALWINES_COORDINATE_H
