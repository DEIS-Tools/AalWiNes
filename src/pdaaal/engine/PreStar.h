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
 * File:   PreStar.h
 * Author: Morten K. Schou <morten@h-schou.dk>
 *
 * Created on 27-01-2020.
 */

#ifndef PRESTAR_H
#define PRESTAR_H

#include "PAutomaton.h"
#include "model/Query.h"

namespace pdaaal {

    class PreStar {
    public:
        PreStar() = default;

        virtual ~PreStar() = default;

        bool verify(const TypedPDA<aalwines::Query::label_t>& pda, bool build_trace) {
            _current_pautomaton = std::make_unique<PAutomaton<aalwines::Query::label_t>>(pda, pda.terminal(), pda.initial_stack());
            _current_pautomaton->_pre_star(); // _current_pautomaton->_pre_star(build_trace); // TODO: implement no-trace version.
            return _current_pautomaton->_accepts(pda.initial(), pda.initial_stack());
        }

        [[nodiscard]] std::vector<TypedPDA<aalwines::Query::label_t>::tracestate_t> get_trace(const PDA& pda) const {
            auto trace = _current_pautomaton->_get_trace(pda.initial(), pda.initial_stack());
            // Remove terminal state from trace.
            trace.pop_back();
            return trace;
        }

    private:
        std::unique_ptr<PAutomaton<aalwines::Query::label_t>> _current_pautomaton = nullptr; // TODO: Maybe change usage interface, so this is not necessary.
    };

}

#endif //PRESTAR_H
