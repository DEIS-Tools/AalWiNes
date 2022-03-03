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
 * File:   more_algorithms.h
 * Author: Morten K. Schou <morten@h-schou.dk>
 *
 * Created on 02-02-2022.
 */

#ifndef AALWINES_MORE_ALGORITHMS_H
#define AALWINES_MORE_ALGORITHMS_H

#include <algorithm>

namespace aalwines::utils {

    // These algorithms are more or less special purpose.
    // Having the logic decoupled from the use makes it easier to test the logic, and it makes the using code more declarative.


    template <typename T, typename TransFn, typename PredFn>
    auto flat_union_if(const std::vector<T>& input, TransFn&& transform, PredFn&& predicate) {
        static_assert(std::is_invocable_v<TransFn,const T&>);
        using U = typename decltype(std::declval<TransFn>()(std::declval<T>()))::value_type;
        static_assert(std::is_convertible_v<TransFn, std::function<std::vector<U>(const T&)>>);
        static_assert(std::is_convertible_v<PredFn, std::function<bool(const T&)>>);
        std::vector<U> result, temp;
        for (const T& elem : input) {
            if (predicate(elem)) {
                std::vector<U> out = transform(elem);
                assert(std::is_sorted(out.begin(), out.end()));
                std::set_union(result.begin(), result.end(), out.begin(), out.end(), std::back_inserter(temp));
                std::swap(temp, result);
                temp.clear();
            }
        }
        return result;
    }
    template <typename T, typename U>
    bool sorted_contains(const std::vector<T>& v, const U& elem) {
        assert(std::is_sorted(v.begin(), v.end()));
        auto lb = std::lower_bound(v.begin(), v.end(), elem);
        return lb != v.end() && *lb == elem;
    }


}

#endif //AALWINES_MORE_ALGORITHMS_H
