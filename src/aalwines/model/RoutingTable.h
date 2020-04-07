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
 * File:   RoutingTable.h
 * Author: Peter G. Jensen <root@petergjoel.dk>
 *
 * Created on July 2, 2019, 3:29 PM
 */
#ifndef ROUTINGTABLE_H
#define ROUTINGTABLE_H

#include <string>
#include <utility>
#include <vector>
#include <map>

#include <ptrie/ptrie_map.h>

#include "Query.h"

namespace aalwines {
    class Router;
    class Interface;
    class Network;
    
    class RoutingTable {
    public:

        enum type_t {
            DISCARD, RECEIVE, ROUTE, MPLS
        };

        enum op_t {
            PUSH, POP, SWAP
        };
        
        using label_t = Query::label_t;

        struct action_t {
            op_t _op = POP;
            label_t _op_label;
            action_t() = default;
            action_t(op_t op, label_t op_label) : _op(op), _op_label(op_label) {};
            void print_json(std::ostream& s, bool quote = true, bool use_hex = true, const Network* network = nullptr) const;
            bool operator==(const action_t& other) const;
            bool operator!=(const action_t& other) const;
        };

        struct forward_t {
            type_t _type = MPLS;
            std::vector<action_t> _ops;
            Interface* _via = nullptr;
            size_t _weight = 0;
            forward_t() = default;
            forward_t(type_t type, std::vector<action_t> ops, Interface* via, size_t weight)
                : _type(type), _ops(std::move(ops)), _via(via), _weight(weight) {};
            void print_json(std::ostream&, bool use_hex = true, const Network* network = nullptr) const;
            friend std::ostream& operator<<(std::ostream& s, const forward_t& fwd);
            bool operator==(const forward_t& other) const;
            bool operator!=(const forward_t& other) const;
            void add_action(action_t action);
        };

        struct entry_t {
            label_t _top_label;
            const Interface* _ingoing = nullptr; // this needs to be removed, it is only really used during merges of Routingtables for filtering
            std::vector<forward_t> _rules;

            bool operator==(const entry_t& other) const;
            bool operator!=(const entry_t& other) const;
            bool operator<(const entry_t& other) const;
            void print_json(std::ostream&) const;
            static void print_label(label_t label, std::ostream& s, bool quote = true);
            friend std::ostream& operator<<(std::ostream& s, const entry_t& entry);
            void clear() { _rules.clear(); }
            void erase_rule(const forward_t& rule){ _rules.erase(std::remove(_rules.begin(), _rules.end(), rule), _rules.end()); }
            void add_to_outgoing(const Interface* outgoing, action_t action);
        };

    public:
        [[nodiscard]] bool empty() const;
        bool overlaps(const RoutingTable& other, Router& parent, std::ostream& warnings) const;
        bool merge(const RoutingTable& other, Interface& parent, std::ostream& warnings);
        void print_json(std::ostream&) const;

        [[nodiscard]] const std::vector<entry_t>& entries() const;
        
        void sort();
        bool check_nondet(std::ostream& e);
        entry_t& push_entry() { _entries.emplace_back(); return _entries.back(); }
        void pop_entry() { _entries.pop_back(); }
        void clear() { _entries.clear(); }
        void erase_entry(RoutingTable::entry_t& entry){ _entries.erase(std::remove(_entries.begin(), _entries.end(), entry), _entries.end()); }


        void add_rules(label_t top_label, const std::vector<forward_t>& rules);
        void add_rule(label_t top_label, const forward_t& rule);
        void add_rule(label_t top_label, forward_t&& rule);
        void add_rule(label_t top_label, action_t op, Interface* via, size_t weight = 0, type_t = MPLS);
        void add_failover_entries(const Interface* failed_inf, Interface* backup_inf, label_t failover_label);
        void add_to_outgoing(const Interface* outgoing, action_t action);
        void add_to_outgoing(const Interface* outgoing, label_t top_label, action_t action);
        std::vector<entry_t>::iterator insert_entry(label_t top_label);
        void simple_merge(const RoutingTable& other);
        
    private:
        std::vector<entry_t> _entries;
    };
}
#endif /* ROUTINGTABLE_H */

