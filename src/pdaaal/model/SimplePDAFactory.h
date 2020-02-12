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
 * File:   SimplePDAFactory.h
 * Author: Morten K. Schou <morten@h-schou.dk>
 *
 * Created on 08-01-2020.
 */

#ifndef PDAAAL_SIMPLEPDAFACTORY_H
#define PDAAAL_SIMPLEPDAFACTORY_H

#include <unordered_set>
#include "TypedPDA.h"

namespace pdaaal {

    template<typename T>
    class SimplePDAFactory {
    protected:
        struct rule_t {
            PDA::op_t _op = PDA::POP;
            T _pre;
            size_t _dest;
            T _op_label;
        };

    public:

        explicit SimplePDAFactory(std::unordered_set<T>&& all_labels)
        : _all_labels(all_labels) { };

        TypedPDA<T> compile() {
            TypedPDA<T> result(_all_labels);
            for (auto state : states()) {
                for (auto& rule : rules(state)) {
                    std::vector<T> pre{rule._pre};
                    result.add_rule(state, rule._dest, rule._op, rule._op_label, false, pre);
                }
            }
            return result;
        }

        virtual const std::vector<size_t>& states() = 0;
        virtual std::vector<rule_t> rules(size_t) = 0;

    protected:
        std::unordered_set<T> _all_labels;
    private:

    };
}

#endif //PDAAAL_SIMPLEPDAFACTORY_H
