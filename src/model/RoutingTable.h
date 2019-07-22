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
 * File:   RoutingTable.h
 * Author: Peter G. Jensen <root@petergjoel.dk>
 *
 * Created on July 2, 2019, 3:29 PM
 */

#include <rapidxml.hpp>
#include <string>
#include <vector>

#include <ptrie_map.h>

#include "Router.h"

#ifndef ROUTINGTABLE_H
#define ROUTINGTABLE_H
namespace mpls2pda {
    class Router;
    class Interface;

    class RoutingTable {
    public:

        enum type_t {
            DISCARD, RECIEVE, ROUTE, MPLS
        };

        enum op_t {
            PUSH, POP, SWAP
        };
        
        using label_t = int64_t;

        struct action_t {
            op_t _op = POP;
            label_t _label = std::numeric_limits<label_t>::min();
            void print_json(std::ostream& s, bool quote = true) const;
            static void print_label(label_t label, std::ostream& s, bool quoted = true);
        };

        struct forward_t {
            type_t _type = MPLS;
            std::vector<action_t> _ops;
            Interface* _via = nullptr;
            size_t _weight = 0;
            void print_json(std::ostream&) const;
            void parse_ops(std::string& opstr);
        };

        struct entry_t {
            label_t _top_label = std::numeric_limits<label_t>::min();
            bool _decreasing = false;
            std::vector<forward_t> _rules;
            bool operator==(const entry_t& other) const;

            bool operator<(const entry_t& other) const;
            void print_json(std::ostream&) const;
            static void print_label(label_t label, std::ostream& s, bool quote = true);

            bool is_interface() const {
                return _top_label < 0;
            }

            bool is_default() const {
                return _top_label == 0;
            }

            bool is_mpls() const {
                return _top_label > 0;
            }

            label_t as_mpls() const {
                return _top_label - 1;
            }

            label_t as_interface() const {
                return -(_top_label + 1);
            }
        };
    public:
        RoutingTable(const RoutingTable& orig) = default;
        virtual ~RoutingTable() = default;
        RoutingTable(RoutingTable&&) = default;
        static RoutingTable parse(rapidxml::xml_node<char>* node, ptrie::map<std::pair<std::string, std::string>>&indirect, Router* parent, std::ostream& warnings);

        RoutingTable& operator=(const RoutingTable&) = default;
        RoutingTable& operator=(RoutingTable&&) = default;

        bool empty() const {
            return _entries.empty();
        }
        bool overlaps(const RoutingTable& other, Router& parent, std::ostream& warnings) const;
        void print_json(std::ostream&) const;

        const std::vector<entry_t>& entries() const {
            return _entries;
        }

        RoutingTable();

    private:

        static Interface* parse_via(Router* parent, rapidxml::xml_node<char>* via);
        static int parse_weight(rapidxml::xml_node<char>* nh);



        std::string _name;
        std::vector<entry_t> _entries;
    };
}
#endif /* ROUTINGTABLE_H */

