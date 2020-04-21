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
    size_t global_id() const {
        return _global_id;
    }
    void set_global_id(size_t global_id) {
        _global_id = global_id;
    }

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
    Router(size_t id, bool is_null = false);
    Router(const Router& orig) = default;
    virtual ~Router() = default;

    size_t index() const {
        return _index;
    }
    void set_index(size_t index) {
        _index = index;
    }
    
    bool is_null() const {
        return _is_null;
    }
    
    void add_name(const std::string& name);
    void change_name(const std::string& name);
    const std::string& name() const;
    const std::vector<std::string>& names() const { return _names; }

    void print_dot(std::ostream& out);
    const std::vector<std::unique_ptr<Interface>>& interfaces() const { return _interfaces; }
    Interface* find_interface(std::string iface);
    Interface* get_interface(std::vector<const Interface*>& all_interfaces, std::string iface, Router* expected = nullptr);
    Interface* interface_no(size_t i) const {
        return _interfaces[i].get();
    }
    std::unique_ptr<char[] > interface_name(size_t i);
    void pair_interfaces(std::vector<const Interface*>&, std::function<bool(const Interface*, const Interface*)> matcher);
    static void add_null_router(std::vector<std::unique_ptr<Router>>& routers, std::vector<const Interface*>& all_interfaces, ptrie::map<char, Router*>& mapping);
    void print_simple(std::ostream& s);
    void print_json(std::ostream& s);

    void set_latitude_longitude(const std::string& latitude, const std::string& longitude);
    const std::string& latitude() const { return _latitude; }
    const std::string& longitude() const { return _longitude; }
private:
    size_t _index = std::numeric_limits<size_t>::max();
    std::vector<std::string> _names;
    std::vector<std::unique_ptr<Interface>> _interfaces;
    ptrie::map<char, Interface*> _interface_map;
    size_t _inamelength = 0; // for printing
    bool _has_config = false;
    bool _is_null = false;
    std::string _latitude;
    std::string _longitude;
    friend class Interface;
};
}
#endif /* ROUTER_H */

