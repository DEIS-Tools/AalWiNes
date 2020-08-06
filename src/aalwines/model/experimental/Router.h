//
// Created by Morten on 05-08-2020.
//

#ifndef AALWINES_ROUTER_H
#define AALWINES_ROUTER_H

#include <limits>
#include <memory>
#include <cassert>
#include <sstream>
#include <aalwines/utils/coordinate.h>
#include <aalwines/model/experimental/RoutingTable.h>

namespace aalwines { namespace experimental {
    class Router;
    class Interface { // TODO: Design Interface class. How is links best represented in AalWiNes. Avoid enforcing bidirectional links.
    public:
        Interface(size_t id, size_t global_id, Router* parent, const std::vector<std::string>& names)
        : _id(id), _global_id(global_id), _parent(parent), _names(names) {};

        RoutingTable& table() { return _table; }
        const RoutingTable& table() const { return _table; }
        size_t id() const {
            return _id;
        }
        size_t global_id() const {
            return _global_id;
        }
        [[nodiscard]] const std::string& name() const {
            _parent->
        }

    private:
        size_t _id = std::numeric_limits<size_t>::max();
        size_t _global_id = std::numeric_limits<size_t>::max();
        Router* _parent = nullptr;
        std::vector<std::string> _names; // TODO: The names can be retrieved from the _parent->_interface_map by using indexes. But we need to have the right indexes... Maybe it is enough to just have one index.
        RoutingTable _table;
    };

    class Router {
    public:
        Router(size_t id, std::vector<std::string> names, std::optional<Coordinate> coordinate)
        : _id(id), _names(std::move(names)), _coordinate(std::move(coordinate)) { };

        [[nodiscard]] const std::string& name() const {
            assert(!_names.empty());
            return _names[0];
        }

        Interface* add_interface(const std::vector<std::string>& interface_names, std::vector<const Interface*>& all_interfaces) {
            assert(interface_names.empty()); // Callers responsibility to throw error.
            /*if (interface_names.empty()) {
                throw base_error("error: Interface must have at least one name.");
            }*/

            auto iid = _interfaces.size();
            auto gid = all_interfaces.size();
            _interfaces.emplace_back(std::make_unique<Interface>(iid, gid, this, interface_names));
            auto interface = _interfaces.back().get();
            all_interfaces.emplace_back(interface);

            for (const auto& interface_name : interface_names) {
                auto res = _interface_map.insert(interface_name.c_str(), interface_name.length());
                if (!res.first) {
                    std::stringstream es;
                    es << "error: Duplicate definition of interface \"" << interface_name << "\" for router \"" << name() << "\".";
                    throw base_error(es.str());
                }
                _interface_map.get_data(res.second) = interface;
            }
            return interface;
        }

        Interface* find_interface(const std::string& interface_name) {
            auto res = _interface_map.exists(interface_name.c_str(), interface_name.length());
            if(!res.first) {
                return nullptr;
            } else {
                return _interface_map.get_data(res.second);
            }
        }

        std::unique_ptr<char[] > interface_name(size_t i) {
            auto n = std::make_unique<char[]>(_inamelength + 1);
            assert(i < _interface_map.size());
            auto res = _interface_map.unpack(i, n.get());
            n[res] = 0;
            return n;
        }

        [[nodiscard]] const std::vector<std::unique_ptr<Interface>>& interfaces() const { return _interfaces; }
        [[nodiscard]] size_t index() const { return _id; }

    private:
        size_t _id = std::numeric_limits<size_t>::max();
        std::vector<std::string> _names;
        std::vector<std::unique_ptr<Interface>> _interfaces;
        std::optional<Coordinate> _coordinate = std::nullopt;

        ptrie::map<char, Interface*> _interface_map;

    };
}
}

#endif //AALWINES_ROUTER_H
