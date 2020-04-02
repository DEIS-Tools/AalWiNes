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
 *  Copyright Morten K. Schou
 */

/*
 * File:   FastRerouting.cpp
 * Author: Morten K. Schou <morten@h-schou.dk>
 *
 * Created on 01-04-2020
 */

#include <queue>
#include <cassert>
#include "FastRerouting.h"

namespace aalwines {

    bool FastRerouting::make_reroute(Network &network, const Interface* failed_inf, label_t failover_label,
            const std::function<uint32_t(const Interface*)>& cost_fn) {
        struct queue_elem {
            queue_elem(uint32_t priority, Interface* interface, const queue_elem* back_pointer = nullptr)
                : priority(priority), interface(interface), back_pointer(back_pointer) { };
            uint32_t priority;
            Interface* interface;
            const queue_elem* back_pointer;
            bool operator<(const queue_elem& other) const {
                return other.priority < priority; // Used in a max-heap, so swap arguments to get a min-heap.
            }
            bool operator==(const queue_elem& other) const {
                return priority == other.priority;
            }
            bool operator!=(const queue_elem& other) const {
                return !(*this == other);
            }
        };
        std::priority_queue<queue_elem> queue;
        std::unordered_set<Router*> seen;
        std::vector<std::unique_ptr<queue_elem>> pointers;
        for(const auto & i : failed_inf->source()->interfaces()) {
            auto interface = i.get();
            if (interface == failed_inf) continue;
            queue.emplace(0, interface);
        }
        seen.emplace(failed_inf->source());
        while (!queue.empty()) {
            auto elem = queue.top();
            queue.pop();
            if (failed_inf->target() == elem.interface->target()){
                auto p = elem.back_pointer;
                assert(p != nullptr);
                // Copy routing table to incoming failover interface.
                elem.interface->match()->table().simple_merge(failed_inf->match()->table());
                // POP at last hop of re-route
                p->interface->match()->table().add_rule(failover_label, RoutingTable::action_t(), elem.interface);
                // SWAP for each intermediate hop during re-route
                auto via = p->interface;
                for (p = p->back_pointer; p != nullptr; via = p->interface, p = p->back_pointer) {
                    p->interface->match()->table().add_rule(failover_label, RoutingTable::action_t(RoutingTable::op_t::SWAP, failover_label), via);
                }
                assert(p == nullptr);
                // PUSH at first hop of re-route
                for (const auto& i : via->source()->interfaces()) {
                    auto interface = i.get();
                    if (interface == failed_inf) continue;
                    interface->table().add_failover_entries(failed_inf, via, failover_label);
                }
                return true;
            }
            if (!seen.emplace(elem.interface->target()).second) continue;
            auto u_pointer = std::make_unique<queue_elem>(elem);
            auto pointer = u_pointer.get();
            pointers.push_back(std::move(u_pointer));
            for(const auto & i : elem.interface->target()->interfaces()) {
                auto interface = i.get();
                if (interface == failed_inf) continue;
                if (interface->target()->is_null()) continue;
                if (seen.count(interface->target()) != 0) continue;
                queue.emplace(elem.priority + cost_fn(interface), interface, pointer);
            }
        }
        return false; // No re-route was possible
    }

    Interface* find_via_interface(const Router* from, const Router* to) {
        for (const auto& i : from->interfaces()) {
            if (i->target() == to){
                return i.get();
            }
        }
        return nullptr;
    }
    Interface* find_interface_on_router(const Router* router, const Interface* interface) {
        // Basically removes const from the Interface*, and checks if the interface is actually on the router.
        if (interface->source() == router) {
            for (const auto& i : router->interfaces()) {
                if (i.get() == interface){
                    return i.get();
                }
            }
        }
        return nullptr;
    }
    bool FastRerouting::make_data_flow(Network &network, const Interface* from, const Interface* to,
            label_t pre_label, label_t flow_label, const std::vector<const Router*>& path) {
        assert(!path.empty());
        // Check if the interfaces is 'outer' interfaces.
        assert(from->target()->is_null());
        assert(to->target()->is_null());

        auto in_interface = find_interface_on_router(path[0], from);
        if (!in_interface) return false;
        if (path.size() == 1) {
            // Simple case: Directly in and out.
            auto out_interface = find_interface_on_router(path[0], to);
            if(!out_interface) return false;
            in_interface->table().add_rule(pre_label, {RoutingTable::op_t::SWAP, pre_label}, out_interface);
            return true;
        }
        // Push flow_label on first hops.
        auto via_interface = find_via_interface(path[0], path[1]);
        if (!via_interface) return false;
        in_interface->table().add_rule(pre_label, {RoutingTable::op_t::PUSH, flow_label}, via_interface);
        // Swap flow_label on intermediate hops.
        size_t i;
        for (i = 1; i + 1 < path.size(); ++i) {
            in_interface = via_interface->match();
            via_interface = find_via_interface(path[i], path[i+1]);
            if (!via_interface) return false;
            in_interface->table().add_rule(flow_label, {RoutingTable::op_t::SWAP, flow_label}, via_interface);
        }
        // Pop flow_label on last hop out.
        in_interface = via_interface->match();
        auto out_interface = find_interface_on_router(path[i], to);
        if(!out_interface) return false;
        in_interface->table().add_rule(flow_label, {RoutingTable::op_t::POP, label_t{}}, out_interface);
        return true;
    }


}
