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
#include "Query.h"

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
        
        using label_t = Query::label_t;

        struct action_t {
            op_t _op = POP;
            label_t _op_label;
            void print_json(std::ostream& s, bool quote = true, bool use_hex = true) const;
        };

        struct forward_t {
            type_t _type = MPLS;
            std::vector<action_t> _ops;
            Interface* _via = nullptr;
            size_t _weight = 0;
            void print_json(std::ostream&, bool use_hex = true) const;
            void parse_ops(std::string& opstr);
            friend std::ostream& operator<<(std::ostream& s, const forward_t& fwd)
            {
                fwd.print_json(s);
                return s;
            }
        };

        struct entry_t {
            label_t _top_label;
            const Interface* _ingoing = nullptr;
            bool _decreasing = false;
            std::vector<forward_t> _rules;
            bool operator==(const entry_t& other) const;

            bool operator<(const entry_t& other) const;
            void print_json(std::ostream&) const;
            static void print_label(label_t label, std::ostream& s, bool quote = true);
            friend std::ostream& operator<<(std::ostream& s, const entry_t& entry)
            {
                s << entry._top_label << " ";
                s << entry._ingoing  << " ";
                entry.print_json(s);
                return s;
            }

        };
    public:
        RoutingTable(const RoutingTable& orig) = default;
        virtual ~RoutingTable() = default;
        RoutingTable(RoutingTable&&) = default;
        static RoutingTable parse(rapidxml::xml_node<char>* node, ptrie::map<std::pair<std::string, std::string>>&indirect, Router* parent, std::vector<const Interface*>& , std::ostream& warnings, bool skip_pfe);

        RoutingTable& operator=(const RoutingTable&) = default;
        RoutingTable& operator=(RoutingTable&&) = default;

        bool empty() const {
            return _entries.empty();
        }
        bool overlaps(const RoutingTable& other, Router& parent, std::ostream& warnings) const;
        bool merge(const RoutingTable& other, Interface& parent, std::ostream& warnings);
        void print_json(std::ostream&) const;

        const std::vector<entry_t>& entries() const {
            return _entries;
        }
        
        const std::string& name() const {
            return _name;
        }

        RoutingTable();

    private:

        static Interface* parse_via(Router* parent, rapidxml::xml_node<char>* via, std::vector<const Interface*>&);
        static int parse_weight(rapidxml::xml_node<char>* nh);



        std::string _name;
        std::vector<entry_t> _entries;
    };
}
#endif /* ROUTINGTABLE_H */

