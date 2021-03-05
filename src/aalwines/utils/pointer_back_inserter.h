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
 * File:   pointer_back_inserter.h
 * Author: Morten K. Schou <morten@h-schou.dk>
 *
 * Created on 01-03-2021.
 */

#ifndef AALWINES_POINTER_BACK_INSERTER_H
#define AALWINES_POINTER_BACK_INSERTER_H

#include <iterator>
#include <type_traits>

// Pretty much same as std::back_inserter, but changed so it inserts a pointer to the element instead of copying (or moving) the element.
template<typename Container>
class pointer_back_insert_iterator : public std::iterator<std::output_iterator_tag, void, void, void, void> {
    static_assert(std::is_pointer_v<typename Container::value_type>, "Container for pointer_back_inserter must have a pointer type as its value type.");
protected:
    Container* container;
public:
    constexpr pointer_back_insert_iterator() noexcept : container(nullptr) { }
    explicit constexpr pointer_back_insert_iterator(Container& x) : container(&x) { }

    constexpr pointer_back_insert_iterator& operator=(const std::remove_pointer_t<typename Container::value_type>& value) {
        container->push_back(&value);
        return *this;
    }
    // Requirements for output iterator.
    constexpr pointer_back_insert_iterator& operator*() { return *this; }
    constexpr pointer_back_insert_iterator& operator++() { return *this; }
    constexpr pointer_back_insert_iterator& operator++(int) { return *this; }
};
template<typename Container>
constexpr inline pointer_back_insert_iterator<Container> pointer_back_inserter(Container& x) {
    return pointer_back_insert_iterator<Container>(x);
}


#endif //AALWINES_POINTER_BACK_INSERTER_H
