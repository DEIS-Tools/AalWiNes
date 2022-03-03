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
 * File:   more_algorithms_test
 * Author: Morten K. Schou <morten@h-schou.dk>
 *
 * Created on 02-02-2022.
 */

#define BOOST_TEST_MODULE more_algorithms_test

#include <boost/test/unit_test.hpp>
#include <aalwines/utils/more_algorithms.h>

using namespace aalwines;

BOOST_AUTO_TEST_CASE(flat_union_if_test_1)
{
    std::vector<int> input{4,1,5,3,2};
    std::unordered_map<int,std::vector<char>> map;
    map.emplace(1, std::vector<char>{'A', 'B', 'C'});
    map.emplace(2, std::vector<char>{});
    map.emplace(3, std::vector<char>{'A'});
    map.emplace(4, std::vector<char>{'C', 'D', 'a'});
    map.emplace(5, std::vector<char>{'X', 'Y', 'Z'});

    auto result = utils::flat_union_if(input, [&map](const auto& e){ return map[e]; }, [](const auto& e){ return e < 5; });

    std::vector<char> expected{'A','B','C','D','a'};

    BOOST_CHECK_EQUAL_COLLECTIONS(result.begin(), result.end(), expected.begin(), expected.end());
}
