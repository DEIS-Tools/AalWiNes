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
 * File:   testmain.cpp
 * Author: Morten K. Schou <morten@h-schou.dk>
 *
 * Created on 03-01-2020.
 */

#include "TestPDAFactory.h"
#include "pdaaal/engine/PAutomaton.h"

using namespace pdaaal;

int main(int argc, const char **argv) {

    TestPDAFactory testFactory{};
    TypedPDA<char> testPDA = testFactory.compile();

    std::vector<char> stack{'A', 'A'};
    PAutomaton test{testPDA, 0, stack};

    test.to_dot_with_symbols(std::cout);

    auto pre_result = test.pre_star();
    pre_result.to_dot_with_symbols(std::cout);

    auto post_result = test.post_star();
    post_result.to_dot_with_symbols(std::cout);

    std::vector<char> test_stack{'B', 'A', 'A', 'A'};
    auto trace = post_result.get_trace(1, test_stack);
    if (trace.empty()) {
        std::cout << "No trace" << std::endl;
    }
    for (auto &trace_state : trace) {
        std::cout << "(" << trace_state._pdastate << ", ";
        for (auto &label : trace_state._stack) {
            std::cout << label;
        }
        std::cout << ")" << std::endl;
    }


    PAutomaton test2{testPDA, 1, test_stack};
    auto pre_result2 = test2.pre_star();
    pre_result2.to_dot_with_symbols(std::cout);

    auto trace2 = pre_result2.get_trace(0, stack);
    if (trace2.empty()) {
        std::cout << "No trace" << std::endl;
    }
    for (auto &trace_state : trace2) {
        std::cout << "(" << trace_state._pdastate << ", ";
        for (auto &label : trace_state._stack) {
            std::cout << label;
        }
        std::cout << ")" << std::endl;
    }

    return 0;
}
