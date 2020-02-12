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
 * File:   TestPDAFactory.cpp
 * Author: Morten K. Schou <morten@h-schou.dk>
 *
 * Created on 03-01-2020.
 */

#include "TestPDAFactory.h"

namespace pdaaal {

    TestPDAFactory::TestPDAFactory()
            : SimplePDAFactory({'A', 'B', 'C'}), _states{0, 1, 2, 3} {

    }

    const std::vector<size_t> &TestPDAFactory::states() {
        return _states;
    }

    std::vector<TestPDAFactory::SimplePDAFactory::rule_t> TestPDAFactory::rules(size_t i) {
        std::vector<TestPDAFactory::SimplePDAFactory::rule_t> rules{};
        switch (i) {
            // This is pretty much the rules from the example in Figure 3.1 (Schwoon-php02)
            // However r_2 requires a swap and a push, which is done through auxiliary state 3.
            case 0:
                rules.push_back({PDA::PUSH, 'A', 1, 'B'});
                rules.push_back({PDA::POP, 'B', 0});
                break;
            case 1:
                rules.push_back({PDA::SWAP, 'B', 3, 'A'});
                break;
            case 2:
                rules.push_back({PDA::SWAP, 'C', 0, 'B'});
                break;
            case 3:
                rules.push_back({PDA::PUSH, 'A', 2, 'C'});
                break;
            default:
                assert(false);
        }
        return rules;
    }

}
