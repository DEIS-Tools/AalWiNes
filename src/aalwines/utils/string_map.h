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
 * File:   string_map.h
 * Author: Morten K. Schou <morten@h-schou.dk>
 *
 * Created on August 7, 2020
 */

#ifndef AALWINES_STRING_MAP_H
#define AALWINES_STRING_MAP_H


#include <ptrie/ptrie_map.h>

/**
 * Convinience class for ptrie::map with string keys.
 * @tparam T is the type of the values stored in the map.
 */
template <typename T>
class string_map : public ptrie::map<char, T> {
    using pt = typename ptrie::map<char, T>;
    using uchar = ptrie::uchar;
public:
    std::pair<bool, size_t> insert(const std::string& key) {
        return pt::insert(key.data(), key.length());
    }

    T& operator[](const std::string& key) {
        return pt::get_data(pt::insert(key.data(), key.length()).second);
    }

    [[nodiscard]] std::pair<bool, size_t> exists(const std::string& key) const {
        return pt::exists(key.data(), key.length());
    }
    bool erase(const std::string& key) {
        return pt::erase(key.data(), key.length());
    }

    [[nodiscard]] std::string at(size_t index) const {
        auto vector = pt::unpack(index);
        return std::string(vector.data(), vector.size());
    }

};


#endif //AALWINES_STRING_MAP_H
