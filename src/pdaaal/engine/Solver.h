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
 * File:   Solver.h
 * Author: Morten K. Schou <morten@h-schou.dk>
 *
 * Created on 12-02-2020.
 */

#ifndef SOLVER_H
#define SOLVER_H

#include "PAutomaton.h"

namespace pdaaal {

    class Solver {
    public:
        Solver() = default;

        virtual ~Solver() = default;

        template <typename T>
        using trace_info = typename std::pair<std::unique_ptr<PAutomaton<T>>, size_t>;
        template <typename T>
        using res_type = typename std::pair<bool, trace_info<T>>;

        template<typename T>
        res_type<T> pre_star(const TypedPDA<T>& pda, bool build_trace) {
            auto automaton = std::make_unique<PAutomaton<T>>(pda, pda.terminal(), pda.initial_stack());
            automaton->_pre_star(); // automaton->_pre_star(build_trace); // TODO: implement no-trace version.
            bool result = automaton->_accepts(pda.initial(), pda.initial_stack());
            return std::make_pair(result, std::make_pair(std::move(automaton), pda.initial()));
        }

        template<typename T>
        res_type<T> post_star(const TypedPDA<T>& pda, bool build_trace) {
            auto automaton = std::make_unique<PAutomaton<T>>(pda, pda.initial(), pda.initial_stack());
            automaton->_post_star(); // automaton->_post_star(build_trace); // TODO: implement no-trace version.
            bool result = automaton->_accepts(pda.terminal(), pda.initial_stack());
            return std::make_pair(result, std::make_pair(std::move(automaton), pda.terminal()));
        }

        template <typename T>
        [[nodiscard]] std::vector<typename TypedPDA<T>::tracestate_t> get_trace(trace_info<T> info) const {
            auto trace = info.first->_get_trace(info.second, info.first->typed_pda().initial_stack());
            trace.pop_back(); // Removes terminal state from trace.
            return trace;
        }
    };

}

#endif /* SOLVER_H */

