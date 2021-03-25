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
 * File:   Router.h
 * Author: Peter G. Jensen <root@petergjoel.dk>
 *
 * Created on June 24, 2019, 11:22 AM
 */

#ifndef ROUTER_H
#define ROUTER_H

#include <limits>
#include <utility>
#include <vector>
#include <string>
#include <memory>

#include <ptrie/ptrie_map.h>
#include <aalwines/utils/coordinate.h>
#include <aalwines/utils/string_map.h>

#include "RoutingTable.h"

#include <aalwines/utils/json_stream.h>
#include <json.hpp>
using json = nlohmann::json;

namespace aalwines {
    class Router;

    class Interface {
        friend class Router;
    public:
        Interface(size_t id, size_t global_id, Router* target, Router* parent)
        : _id(id), _global_id(global_id), _target(target), _parent(parent) {};
        Interface(size_t id, size_t global_id, Router* parent)
        : _id(id), _global_id(global_id), _parent(parent) {};

        [[nodiscard]] Router* target() const {
            return _target;
        }

        [[nodiscard]] Router* source() const {
            return _parent;
        }

        RoutingTable* table() {
            return _table;
        }

        [[nodiscard]] const RoutingTable* table() const {
            return _table;
        }
        void set_table(RoutingTable* table) {
            _table = table;
            _table->add_my_interface(this);
        }

        [[nodiscard]] bool is_virtual() const {
            return _parent == _target;
        }

        [[nodiscard]] size_t id() const {
            return _id;
        }
        [[nodiscard]] size_t global_id() const {
            return _global_id;
        }
        void set_global_id(size_t global_id) {
            _global_id = global_id;
        }
        [[nodiscard]] std::string get_name() const;
        void make_pairing(Interface* interface);
        [[nodiscard]] Interface* match() const { return _matching; }
    private:
        size_t _id = std::numeric_limits<size_t>::max();
        size_t _global_id = std::numeric_limits<size_t>::max();
        Router* _target = nullptr;
        Router* _parent = nullptr;
        Interface* _matching = nullptr;
        RoutingTable* _table = nullptr;
    };

    class Router {
        friend class Interface;
    public:
        explicit Router(size_t id, bool is_null = false) : _index(id), _is_null(is_null) { };
        Router(size_t id, std::vector<std::string> names, std::optional<Coordinate> coordinate = std::nullopt)
        : _index(id), _names(std::move(names)), _coordinate(std::move(coordinate)) { };
        Router(size_t id, std::vector<std::string> names, bool is_null)
        : _index(id), _names(std::move(names)), _is_null(is_null) { };

        virtual ~Router() = default;
        Router(const Router& router) {
            *this = router;
        };
        Router& operator=(const Router& router);
        Router(Router&&) = default;
        Router& operator=(Router&&) = default;

        [[nodiscard]] size_t index() const {
            return _index;
        }
        void set_index(size_t index) {
            _index = index;
        }

        [[nodiscard]] bool is_null() const {
            return _is_null;
        }

        void add_name(const std::string& name);
        void change_name(const std::string& name);
        [[nodiscard]] const std::string& name() const;
        [[nodiscard]] const std::vector<std::string>& names() const { return _names; }

        [[nodiscard]] const std::vector<std::unique_ptr<Interface>>& interfaces() const { return _interfaces; }
        std::pair<bool,Interface*> insert_interface(const std::string& interface_name, std::vector<const Interface*>& all_interfaces, bool make_table = true);
        Interface* get_interface(const std::string& interface_name, std::vector<const Interface*>& all_interfaces);
        Interface* find_interface(const std::string& interface_name);
        [[nodiscard]] std::string interface_name(size_t i) const;

        RoutingTable* emplace_table() { return _tables.emplace_back(std::make_unique<RoutingTable>()).get(); }
        [[nodiscard]] const std::vector<std::unique_ptr<RoutingTable>>& tables() const { return _tables; }

        void print_dot(std::ostream& out) const;
        void print_simple(std::ostream& s) const;
        void print_json(json_stream& json_output) const;
        [[nodiscard]] size_t count_rules() const;
        [[nodiscard]] size_t count_entries() const;

        void set_latitude_longitude(const std::string& latitude, const std::string& longitude);
        [[nodiscard]] std::string latitude() const {return _coordinate ? std::to_string(_coordinate->latitude()) : ""; };
        [[nodiscard]] std::string longitude() const {return _coordinate ? std::to_string(_coordinate->longitude()) : ""; };
        [[nodiscard]] std::optional<Coordinate> coordinate() const { return _coordinate; }
        void set_coordinate(Coordinate coordinate) { _coordinate.emplace(coordinate); }

        // Check sanity of network data structure
        bool check_sanity(std::ostream& error_stream = std::cerr) const;
        // Remove redundant rules.
        void pre_process(std::ostream& log = std::cerr);

    private:
        size_t _index = std::numeric_limits<size_t>::max();
        std::vector<std::string> _names;
        std::optional<Coordinate> _coordinate = std::nullopt;
        bool _is_null = false;
        std::vector<std::unique_ptr<Interface>> _interfaces;
        std::vector<std::unique_ptr<RoutingTable>> _tables;
        string_map<Interface*> _interface_map;
    };
}
#endif /* ROUTER_H */

