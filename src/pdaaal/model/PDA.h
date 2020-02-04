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
 *  Copyright Peter G. Jensen
 */

/* 
 * File:   PDA.h
 * Author: Peter G. Jensen <root@petergjoel.dk>
 *
 * Created on August 21, 2019, 2:47 PM
 */

#ifndef PDA_H
#define PDA_H

#include <set>
#include <cinttypes>
#include <vector>

namespace pdaaal {

    class PDA {
    public:

        enum op_t {
            PUSH = 1,
            POP = 2,
            SWAP = 4,
            NOOP = 8
        };
        struct tos_t;

        struct pre_t {
        private:
            bool _wildcard = false;
            std::vector<uint32_t> _labels;
        public:

            bool wildcard() const;

            const std::vector<uint32_t>& labels() const;
            void merge(bool negated, const std::vector<uint32_t>& other, size_t all_labels);

            void clear();

            bool empty() const;
            bool intersect(const tos_t& tos, size_t all_labels);
            bool noop_pre_filter(const std::set<uint32_t>& usefull);
            bool contains(uint32_t label) const;
        };

        struct rule_t {
            size_t _to = 0;
            op_t _operation = NOOP;
            uint32_t _op_label = uint32_t{};

            bool is_terminal() const;

            bool operator<(const rule_t& other) const;

            bool operator==(const rule_t& other) const;

            bool operator!=(const rule_t& other) const;

            pre_t _precondition;
        };

        struct state_t {
            std::vector<rule_t> _rules;
            std::vector<size_t> _pre;
        };

        struct tos_t {
            std::vector<uint32_t> _tos; // TODO: some symbolic representation would be better here
            std::vector<uint32_t> _stack; // TODO: some symbolic representation would be better here

            enum waiting_t {
                NOT_IN_STACK = 0, TOS = 8, BOS = 16, BOTH = TOS | BOS
            };
            waiting_t _in_waiting = NOT_IN_STACK;

            bool update_state(const std::pair<bool, bool>& new_state);

            bool empty_tos() const;

            bool active(const tos_t& prev, const rule_t& rule);

            bool forward_stack(const tos_t& prev, size_t all_labels);

            std::pair<bool, bool> merge_pop(const tos_t& prev, const rule_t& rule, bool dual_stack, size_t all_labels);

            std::pair<bool, bool> merge_noop(const tos_t& prev, const rule_t& rule, bool dual_stack, size_t all_labels);

            std::pair<bool, bool> merge_swap(const tos_t& prev, const rule_t& rule, bool dual_stack, size_t all_labels);

            std::pair<bool, bool> merge_push(const tos_t& prev, const rule_t& rule, bool dual_stack, size_t all_labels);
        };

        void forwards_prune();

        void backwards_prune();

        void target_tos_prune();

        std::pair<size_t, size_t> reduce(int aggresivity);

        size_t size() const;

        virtual size_t number_of_labels() const = 0;

        const std::vector<state_t>& states() const;
        const size_t initial() const { return _initial_id; }
        const std::vector<uint32_t>& initial_stack() const { return _initial_stack; }
        const size_t terminal() const { return 0; }
        virtual ~PDA();

    protected:
        PDA();
    protected:
        void _add_rule(size_t from, size_t to, op_t op, uint32_t label, bool negated, const std::vector<uint32_t>& pre);
        void clear_state(size_t s);
        void finalize();

    protected:
        std::vector<state_t> _states;
        const std::vector<uint32_t> _initial_stack{std::numeric_limits<uint32_t>::max()};
        state_t _initial;
        size_t _initial_id = 0;
    };

}
#endif /* PDA_H */

