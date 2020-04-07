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
#include <vector>
#include <string>
#include <memory>

#include <ptrie/ptrie_map.h>
#include "aalwines/utils/coordinate.h"

#include "RoutingTable.h"

namespace aalwines {
class Interface {
public:
    Interface(size_t id, size_t global_id, Router* target, Router* parent);

    Router* target() const {
        return _target;
    }
    
    Router* source() const {
        return _parent;
    }
    
    RoutingTable& table() {
        return _table;
    }
    
    const RoutingTable& table() const {
        return _table;
    }
    
    bool is_virtual() const {
        return _parent == _target;
    }

    size_t id() const {
        return _id;
    }

    void update_id(uint64_t id_value) {
        _id = id_value;
    }

    size_t global_id() const {
        return _global_id;
    }
    void update_global_id(size_t global_id) {
        _global_id = global_id;
    }
    
    void remove_pairing(Interface* interface);
    void make_pairing(Interface* interface);
    void make_pairing(std::vector<const Interface*>& all_interfaces, std::function<bool(const Interface*, const Interface*)> matcher);
    Interface* match() const { return _matching; }
private:
    size_t _id = std::numeric_limits<size_t>::max();
    size_t _global_id = std::numeric_limits<size_t>::max();
    Router* _target = nullptr;
    Interface* _matching = nullptr;
    Router* _parent = nullptr;
    RoutingTable _table;
};

class Router {
public:
    explicit Router(size_t id, bool is_null = false);
    Router(size_t id, Coordinate coordinate);
    Router(const Router& orig) = default;
    virtual ~Router() = default;

    [[nodiscard]] size_t index() const {
        return _index;
    }
    void update_index(size_t index) {
        _index = index;
    }
    
    [[nodiscard]] bool is_null() const {
        return _is_null;
    }

    Interface* get_null_interface() {
        for(auto& inf : _interfaces){
            if(inf->target()->is_null()){
                return inf.get();
            }
        }
    }
    
    void add_name(const std::string& name);
    void change_name(const std::string& name);
    [[nodiscard]] const std::string& name() const;
    [[nodiscard]] const std::vector<std::string>& names() const { return _names; }

    void print_dot(std::ostream& out);
    [[nodiscard]] const std::vector<std::unique_ptr<Interface>>& interfaces() const { return _interfaces; }
    void remove_interface(Interface* interface);
    Interface* find_interface(std::string iface);
    Interface* get_interface(std::vector<const Interface*>& all_interfaces, std::string iface, Router* expected = nullptr);
    [[nodiscard]] Interface* interface_no(size_t i) const {
        return _interfaces[i].get();
    }
    std::unique_ptr<char[] > interface_name(size_t i);
    void pair_interfaces(std::vector<const Interface*>&, std::function<bool(const Interface*, const Interface*)> matcher);
    static void add_null_router(std::vector<std::unique_ptr<Router>>& routers, std::vector<const Interface*>& all_interfaces, ptrie::map<char, Router*>& mapping);
    void print_simple(std::ostream& s);
    [[nodiscard]] std::optional<Coordinate> coordinate() const { return _coordinate; }
private:
    size_t _index = std::numeric_limits<size_t>::max();
    std::vector<std::string> _names;
    std::vector<std::unique_ptr<Interface>> _interfaces;
    ptrie::map<char, Interface*> _interface_map;
    size_t _inamelength = 0; // for printing
    bool _has_config = false;
    bool _is_null = false;
    std::optional<Coordinate> _coordinate = std::nullopt;
    friend class Interface;
};
}
#endif /* ROUTER_H */

