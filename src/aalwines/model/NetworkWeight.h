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
 *  Copyright Dan Kristiansen, Morten K. Schou
 */

/*
 * File:   NetworkWeight.h
 * Author: Dan Kristiansen & Morten K. Schou
 *
 * Created on March 16, 2020
 */

#ifndef AALWINES_NETWORKWEIGHT_H
#define AALWINES_NETWORKWEIGHT_H

#include <pdaaal/Weight.h>
#include <json.hpp>

#include <utility>
#include <fstream>

using json = nlohmann::json;

namespace aalwines {

    /**
     * Weight language JSON syntax:
     * [ // Outer array: In order of priority
     *   [ // Inner array: Linear combination of atoms.
     *       {"atom": ATOM,
     *        "factor": NUM // (optional)
     *       }, ...
     *   ], ...
     * ]
     * where
     *  - ATOM = {"links", "hops", "distance", "local_failures", "tunnels", "custom", "latency", "zero"}
     *  - NUM = {0,1,2,...}
     */
    class NetworkWeight {
    private:
        using atomic_property_function = std::function<uint32_t(const RoutingTable::forward_t&, const RoutingTable::entry_t&)>;
        using linear_weight_function = pdaaal::linear_weight_function<uint32_t, const RoutingTable::forward_t&, const RoutingTable::entry_t&>;
    public:
        using weight_function = pdaaal::ordered_weight_function<uint32_t, const RoutingTable::forward_t&, const RoutingTable::entry_t&>;

        enum class AtomicProperty {
            default_weight_function,
            number_of_links,
            number_of_hops,
            distance,
            local_failures,
            tunnels,
            push_ops,
            custom,
            //latency,
        };

        NetworkWeight() = default;
        //explicit NetworkWeight(std::unordered_map<const Interface*, uint32_t>  latency_map) : _latency_map(std::move(latency_map)) {};

        [[nodiscard]] atomic_property_function get_atom(AtomicProperty atom) const {
            switch (atom) {
                case AtomicProperty::number_of_links:
                    return [](const RoutingTable::forward_t& r, const RoutingTable::entry_t& _) -> uint32_t {
                        return 1;
                    };
                case AtomicProperty::number_of_hops: // Does not count links that are self-loops.
                    return [](const RoutingTable::forward_t& r, const RoutingTable::entry_t& _) -> uint32_t {
                        return !r._via->is_virtual() ? 1 : 0;
                    };
                case AtomicProperty::distance:
                    return [](const RoutingTable::forward_t& r, const RoutingTable::entry_t& _) -> uint32_t {
                        return r._via->source()->coordinate() && r._via->target()->coordinate()
                               ? r._via->source()->coordinate()->distance_to(r._via->target()->coordinate().value())
                               : 20038; //(km). If coordinates are missing, use half circumference of earth, i.e. worst case distance.
                    };
                case AtomicProperty::local_failures:
                    return [](const RoutingTable::forward_t& r, const RoutingTable::entry_t& e) -> uint32_t {
                        std::unordered_set<const Interface*> edges;
                        for (const auto& other : e._rules) {
                            if (other._priority < r._priority) {
                                edges.emplace(other._via);
                            }
                        }
                        return edges.size();
                    };
                case AtomicProperty::tunnels:
                    return [](const RoutingTable::forward_t& r, const RoutingTable::entry_t& _) -> uint32_t {
                        auto push_ops = std::count_if(r._ops.begin(), r._ops.end(), [](RoutingTable::action_t act) -> bool { return act._op == RoutingTable::op_t::PUSH; });
                        auto pop_ops = std::count_if(r._ops.begin(), r._ops.end(), [](RoutingTable::action_t act) -> bool { return act._op == RoutingTable::op_t::POP; });
                        return push_ops > pop_ops ? push_ops - pop_ops : 0;
                    };
                case AtomicProperty::push_ops:
                    return [](const RoutingTable::forward_t& r, const RoutingTable::entry_t& _) -> uint32_t {
                        return std::count_if(r._ops.begin(), r._ops.end(), [](RoutingTable::action_t act) -> bool { return act._op == RoutingTable::op_t::PUSH; });
                    };
                case AtomicProperty::custom:
                    return [](const RoutingTable::forward_t& r, const RoutingTable::entry_t& _) -> uint32_t {
                        return r._weight;
                    };
/*                case AtomicProperty::latency:
                    return [this](const RoutingTable::forward_t& r, const RoutingTable::entry_t& _) -> uint32_t {
                        auto it = this->_latency_map.find(r._via);
                        return it != this->_latency_map.end() ? it->second : 0;
                        // (r._via->source()->index(), r._via->target()->index())
                    };*/
                case AtomicProperty::default_weight_function:
                default:
                    return [](const RoutingTable::forward_t& r, const RoutingTable::entry_t& _) -> uint32_t {
                        return 0;
                    };
            }
        }

        auto parse(std::istream &stream) const {
            json j;
            stream >> j;

            if (!j.is_array()) {
                throw base_error("No outer array. ");
            }
            std::vector<linear_weight_function> fns;
            for (auto& elem : j) {
                if (elem.size() == 1 && (!elem[0].contains("factor") || elem[0]["factor"] == 1)) {
                    fns.emplace_back(parse_atom(elem[0]));
                } else {
                    fns.emplace_back(parse_lin_exp(elem));
                }
            }
            return pdaaal::ordered_weight_function(fns);
        }

        static auto default_weight_fn() {
            return pdaaal::ordered_weight_function(std::vector<linear_weight_function>{linear_weight_function{[](const RoutingTable::forward_t& r, const RoutingTable::entry_t& _) -> uint32_t {
                return 0;
            }}});
        }

    private:
        atomic_property_function parse_atom(const json& elem) const {
            if (!elem.contains("atom")) {
                throw base_error("Object of inner array has no key \"atom\". ");
            }
            auto atom = elem["atom"];
            if (!atom.is_string()) {
                throw base_error("Atomic property field is not a string. ");
            }
            auto s = atom.get<std::string>();
            AtomicProperty p;
            if (s == "links") {
                p = AtomicProperty::number_of_links;
            } else if (s == "hops") {
                p = AtomicProperty::number_of_hops;
            } else if (s == "distance") {
                p = AtomicProperty::distance;
            } else if (s == "local_failures" || s == "failures") { // Support both namings
                p = AtomicProperty::local_failures;
            } else if (s == "tunnels") {
                p = AtomicProperty::tunnels;
            } else if (s == "custom") {
                p = AtomicProperty::custom;
            } else if (s == "latency") {
                p = AtomicProperty::custom; // Currently latency info is annotated as the custom weights.
            } else if (s == "zero") {
                p = AtomicProperty::default_weight_function;
            } else {
                throw base_error("Unknown atomic property: " + s);
            }
            return get_atom(p);
        }
        std::vector<std::pair<uint32_t,linear_weight_function>> parse_lin_exp(const json& inner) const {
            if (!inner.is_array()) {
                throw base_error("Inner element is not an array. ");
            }
            std::vector<std::pair<uint32_t,linear_weight_function>> result;
            for (auto& elem : inner) {
                if (!elem.is_object()) {
                    throw base_error("Element of inner array is not an object. ");
                }
                auto factor = elem.contains("factor") ? elem["factor"].get<uint32_t>() : 1;
                result.emplace_back(factor, pdaaal::linear_weight_function(parse_atom(elem)));
            }
            return result;
        }

    private:
        //std::unordered_map<const Interface*, uint32_t> _latency_map;
    };

}

#endif //AALWINES_NETWORKWEIGHT_H
