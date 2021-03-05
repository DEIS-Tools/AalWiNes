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

#include <json.hpp>
#include <ptrie/ptrie_map.h>

#include "Query.h"
#include <pdaaal/PDA.h>

using json = nlohmann::json;

namespace aalwines {
    class Interface;
    class Network;
    
    class RoutingTable {
    public:
        enum class op_t {
            PUSH, POP, SWAP
        };
        
        using label_t = Query::label_t;

        struct action_t {
            op_t _op = op_t::POP;
            size_t _op_label = std::numeric_limits<size_t>::max();
            action_t() = default;
            explicit action_t(op_t op) : _op(op) {
                assert(op == op_t::POP);
            };
            action_t(op_t op, size_t op_label) : _op(op), _op_label(op_label) {
                assert(op == op_t::PUSH || op == op_t::SWAP);
            };
            action_t(op_t op, const std::string& op_label) : _op(op) {
                assert(op == op_t::PUSH || op == op_t::SWAP);
                _op_label = static_cast<size_t>(std::stoul(op_label));
            };
            void print_json(std::ostream& s, bool quote = true, bool use_hex = true, const Network* network = nullptr) const;
            [[nodiscard]] json to_json() const;
            bool operator==(const action_t& other) const;
            bool operator!=(const action_t& other) const;
            [[nodiscard]] std::pair<pdaaal::op_t,label_t> convert_to_pda_op() const;
            std::pair<pdaaal::op_t,size_t> convert_to_pda_op(const std::function<std::pair<bool,size_t>(const label_t&)>& label_abstraction) const;
        };

        struct forward_t {
            std::vector<action_t> _ops;
            Interface* _via = nullptr;
            size_t _priority = 0;
            uint32_t _weight = 0;
            forward_t() = default;
            forward_t(std::vector<action_t> ops, Interface* via, size_t priority, uint32_t weight = 0)
                : _ops(std::move(ops)), _via(via), _priority(priority), _weight(weight) {};
            void print_json(std::ostream&, bool use_hex = true, const Network* network = nullptr) const;
            [[nodiscard]] json to_json() const;
            friend std::ostream& operator<<(std::ostream& s, const forward_t& fwd);
            bool operator==(const forward_t& other) const;
            bool operator!=(const forward_t& other) const;
            void add_action(action_t action);
            [[nodiscard]] std::pair<pdaaal::op_t,label_t> first_action() const;
            std::pair<pdaaal::op_t,size_t> first_action(const std::function<std::pair<bool,size_t>(const label_t&)>& label_abstraction) const;
        };

        struct entry_t {
            size_t _top_label = Query::wildcard_label();
            std::vector<forward_t> _rules;

            entry_t() = default;
            explicit entry_t(size_t top_label) : _top_label{top_label} { };
            explicit entry_t(const std::string& label) {
                if (!label.empty() && label != "null") {
                    _top_label = static_cast<size_t>(std::stoul(label));
                }
            }

            bool operator==(const entry_t& other) const;
            bool operator!=(const entry_t& other) const;
            bool operator<(const entry_t& other) const;
            void print_json(std::ostream&) const;
            static void print_label(label_t label, std::ostream& s, bool quote = true);
            friend std::ostream& operator<<(std::ostream& s, const entry_t& entry);
            void add_to_outgoing(const Interface* outgoing, action_t action);
            [[nodiscard]] bool ignores_label() const {
                return _top_label == Query::wildcard_label();
            }
        };

    public:
        [[nodiscard]] bool empty() const;
        void print_json(std::ostream&) const;

        [[nodiscard]] const std::vector<entry_t>& entries() const;
        
        void sort();
        template <typename... Args>
        entry_t& emplace_entry(Args... args) { return _entries.emplace_back(std::forward<Args>(args)...); }
        void pop_entry() { _entries.pop_back(); }
        entry_t& back() { return _entries.back(); }

        void add_rules(label_t top_label, const std::vector<forward_t>& rules);
        void add_rule(label_t top_label, const forward_t& rule);
        void add_rule(label_t top_label, forward_t&& rule);
        void add_rule(label_t top_label, action_t op, Interface* via, size_t weight = 0);
        void add_failover_entries(const Interface* failed_inf, Interface* backup_inf, label_t failover_label);
        void add_to_outgoing(const Interface* outgoing, action_t action);
        void merge(const RoutingTable& other);

        void update_interfaces(const std::function<Interface*(const Interface*)>& update_fn);
        
    private:
        std::vector<entry_t>::iterator insert_entry(label_t top_label);

        std::vector<entry_t> _entries;
    };
}
#endif /* ROUTINGTABLE_H */

