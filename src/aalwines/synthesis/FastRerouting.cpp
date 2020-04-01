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
#include "FastRerouting.h"

namespace aalwines {

    bool FastRerouting::make_reroute(Network &network, const Interface* inf, label_t fresh){
        struct queue_elem {
            queue_elem(int priority, Interface* interface, const queue_elem* back_pointer = nullptr)
                : priority(priority), interface(interface), back_pointer(back_pointer) { };
            int priority;
            Interface* interface;
            const queue_elem* back_pointer;
            bool operator<(const queue_elem& other) const {
                return priority < other.priority;
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
        for(const auto & i : inf->source()->interfaces()) {
            auto interface = i.get();
            if (interface == inf) continue;
            queue.emplace(0, interface);
        }
        while (!queue.empty()) {
            auto elem = queue.top();
            queue.pop();
            if (inf->target() == elem.interface->target()){
                auto p = elem.back_pointer;
                assert(p != nullptr);
                { // POP at last hop of re-route
                    RoutingTable table;
                    auto entry = table.push_entry();
                    auto ingoing_interface = p->interface->match();
                    entry._ingoing = ingoing_interface;
                    entry._top_label = fresh;
                    entry._rules.emplace_back();
                    entry._rules.back()._ops.emplace_back();
                    entry._rules.back()._via = elem.interface;
                    ingoing_interface->table().merge(table, *ingoing_interface, std::cerr);
                }
                // SWAP for each intermediate hop during re-route
                auto via = p->interface;
                for (p = p->back_pointer; p != nullptr; via = p->interface, p = p->back_pointer) {
                    RoutingTable table;
                    auto entry = table.push_entry();
                    auto ingoing_interface = p->interface->match();
                    entry._ingoing = ingoing_interface;
                    entry._top_label = fresh;
                    entry._rules.emplace_back();
                    entry._rules.back()._ops.emplace_back();
                    entry._rules.back()._ops.back()._op = RoutingTable::op_t::SWAP;
                    entry._rules.back()._ops.back()._op_label = fresh;
                    entry._rules.back()._via = via;
                    ingoing_interface->table().merge(table, *ingoing_interface, std::cerr);
                }
                assert(p == nullptr);
                // PUSH at first hop of re-route
                // TODO: Make all this a lot easier by implementing advanced modification operations in routing table.
                for (const auto& i : via->source()->interfaces()) {
                    auto interface = i.get();
                    if (interface == inf) continue;
                    RoutingTable table;
                    for (const auto& e : interface->table().entries()) {
                        for (const auto& f : e._rules) {
                            if (f._via == interface) {
                                auto entry = table.push_entry();
                                entry._ingoing = interface;
                                entry._top_label = e._top_label;
                                entry._rules.emplace_back();
                                entry._rules.back()._ops.emplace_back();
                                entry._rules.back()._ops.back()._op = RoutingTable::op_t::PUSH;
                                entry._rules.back()._ops.back()._op_label = fresh;
                                entry._rules.back()._via = via;
                                entry._rules.back()._weight = f._weight + 1;
                            }
                        }
                    }
                    table.sort();
                    interface->table().merge(table, *interface, std::cerr);
                }
                return true;
            }
            if (!seen.emplace(elem.interface->target()).second) continue;
            auto u_pointer = std::make_unique<queue_elem>(elem);
            auto pointer = u_pointer.get();
            pointers.push_back(std::move(u_pointer));
            for(const auto & i : elem.interface->target()->interfaces()) {
                auto interface = i.get();
                if (interface == inf) continue;
                if (seen.count(interface->target()) != 0) continue;
                queue.emplace(elem.priority + 1, interface, pointer); // TODO: Implement weight/cost instead of 1.
            }
        }
        return false; // No re-route was possible
    }

}
