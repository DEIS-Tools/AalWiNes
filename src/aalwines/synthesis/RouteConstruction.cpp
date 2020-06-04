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
 * File:   RouteConstruction.cpp
 * Author: Morten K. Schou <morten@h-schou.dk>
 *
 * Created on 01-04-2020
 */

#include <queue>
#include <cassert>
#include "RouteConstruction.h"

namespace aalwines {

    template <typename Edge, typename Weight>
    struct queue_elem {
        queue_elem(uint32_t priority, Edge edge, const queue_elem<Edge,Weight>* back_pointer = nullptr)
                : priority(priority), edge(edge), back_pointer(back_pointer) { };
        Weight priority;
        Edge edge;
        const queue_elem<Edge,Weight>* back_pointer;
        bool operator<(const queue_elem<Edge,Weight>& other) const {
            return other.priority < priority; // Used in a max-heap, so swap arguments to get a min-heap.
        }
    };
    template <typename Edge, typename Weight, typename Node, typename EdgeFn, typename TargetFn, typename AcceptFn, typename FilterFn, typename CostFn>
    std::optional<queue_elem<Edge,Weight>> dijkstra(
            std::vector<std::unique_ptr<queue_elem<Edge,Weight>>>& pointers,
            Node start_node, EdgeFn&& edges, TargetFn&& target_node, AcceptFn&& accept, FilterFn&& filter_out, CostFn&& cost_fn) {
        static_assert(std::is_convertible_v<EdgeFn, std::function<std::vector<Edge>(Node)>>);
        static_assert(std::is_convertible_v<TargetFn, std::function<Router*(Edge)>>);
        static_assert(std::is_convertible_v<AcceptFn, std::function<bool(Edge)>>);
        static_assert(std::is_convertible_v<FilterFn, std::function<bool(Edge)>>);
        static_assert(std::is_convertible_v<CostFn, std::function<Weight(Edge)>>);
        std::priority_queue<queue_elem<Edge,Weight>> queue;
        std::unordered_set<Node> seen;
        for(auto&& edge : edges(start_node)) {
            if (filter_out(edge)) continue;
            queue.emplace(0, edge);
        }
        seen.emplace(start_node);
        while (!queue.empty()) {
            auto elem = queue.top();
            queue.pop();
            if (accept(elem.edge)){
                return elem;
            }
            if (!seen.emplace(target_node(elem.edge)).second) continue;
            auto u_pointer = std::make_unique<queue_elem<Edge,Weight>>(elem);
            auto pointer = u_pointer.get();
            pointers.push_back(std::move(u_pointer));
            for(auto&& edge : edges(target_node(elem.edge))) {
                if (filter_out(edge) || seen.count(target_node(edge)) != 0) continue;
                queue.emplace(elem.priority + cost_fn(edge), edge, pointer);
            }
        }
        return std::nullopt; // No path was found
    }

    bool RouteConstruction::make_reroute(const Interface* failed_inf, const std::function<label_t(void)>& next_label,
                                         const std::function<uint32_t(const Interface*)>& cost_fn) {
        std::vector<std::unique_ptr<queue_elem<Interface*,uint32_t>>> pointers;
        auto val = dijkstra(pointers, failed_inf->source(),
            [](const Router* node) { // Get edges in node
                std::vector<Interface*> result(node->interfaces().size());
                std::transform(node->interfaces().begin(), node->interfaces().end(), result.begin(),
                        [](const auto &i){ return i.get(); });
                return result; // TODO: When C++20 comes with std::ranges, use a transform_view
            },
            [](const Interface* interface) { return interface->target(); }, // Get target of edge
            [failed_target = failed_inf->target()](const Interface* interface) {
                return failed_target == interface->target(); // Accept if we found the target of failed_inf
            },
            [failed_inf](const Interface* interface) { // Don't use failed_inf and the null router.
                return interface == failed_inf || interface->target()->is_null();
            }, cost_fn);
        if (!val) return false;
        auto elem = val.value();
        auto p = elem.back_pointer;
        assert(p != nullptr);
        // Copy routing table to incoming failover interface.
        elem.edge->match()->table().simple_merge(failed_inf->match()->table());
        // POP at last hop of re-route
        auto label = next_label();
        p->edge->match()->table().add_rule(label, RoutingTable::action_t(RoutingTable::op_t::POP, label_t{}), elem.edge);
        // SWAP for each intermediate hop during re-route
        auto via = p->edge;
        for (p = p->back_pointer; p != nullptr; via = p->edge, p = p->back_pointer) {
            auto old_label = label;
            label = next_label();
            p->edge->match()->table().add_rule(label, RoutingTable::action_t(RoutingTable::op_t::SWAP, old_label), via);
        }
        assert(p == nullptr);
        // PUSH at first hop of re-route
        for (const auto& i : via->source()->interfaces()) {
            auto interface = i.get();
            if (interface == failed_inf) continue;
            interface->table().add_failover_entries(failed_inf, via, label);
        }
        return true;
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
    bool RouteConstruction::make_data_flow(const Interface* from, const Interface* to,
                                           const std::function<label_t(void)>& next_label, const std::vector<const Router*>& path) {
        assert(!path.empty());
        // Check if the interfaces is 'outer' interfaces.
        assert(from->target()->is_null());
        assert(to->target()->is_null());
        std::vector<Interface*> interface_path;
        auto in_interface = find_interface_on_router(path[0], from);
        if (!in_interface) return false;
        for (size_t i = 0; i < path.size() - 1; ++i) {
            auto interface = find_via_interface(path[i], path[i+1]);
            if (!interface) return false;
            interface_path.push_back(interface);
        }
        auto out_interface = find_interface_on_router(path[path.size()-1], to);
        if (!out_interface) return false;
        interface_path.push_back(out_interface);
        return make_data_flow(in_interface, interface_path, next_label);
    }

    bool RouteConstruction::make_data_flow(Interface* from, const std::vector<Interface*>& path,
                                           const std::function<label_t(void)>& next_label) {
        assert(!path.empty());
        // Check if the interfaces is 'outer' interfaces.
        assert(from->target()->is_null());
        assert(path[path.size()-1]->target()->is_null());
        auto pre_label = next_label();        
        bool first = true;
        // Swap labels on hops.
        for (auto via : path) {
            if (first){
                pre_label.set_type(Query::ANYIP);
                from->table().add_rule(pre_label, {RoutingTable::op_t::PUSH, next_label()}, via);
                first = false;
                continue;
            }
            if (from->source() != via->source()) return false;
            auto swap_label = next_label();
            from->table().add_rule(pre_label, {RoutingTable::op_t::SWAP, swap_label}, via);
            from = via->match();
            pre_label = swap_label;
        }
        return true;
    }

    bool RouteConstruction::make_data_flow(Interface* from, Interface* to, const std::function<label_t(void)>& next_label,
                                           const std::function<uint32_t(const Interface*)>& cost_fn) {
        if (from->source() == to->source()) {
            return make_data_flow(from, std::vector<Interface*>{to}, next_label);
        }
        std::vector<std::unique_ptr<queue_elem<Interface *, uint32_t>>> pointers;
        auto val = dijkstra(pointers, from->source(),
                            [](const Router *node) { // Get edges in node
                                std::vector<Interface *> result(node->interfaces().size());
                                std::transform(node->interfaces().begin(), node->interfaces().end(), result.begin(),
                                               [](const auto &i) { return i.get(); });
                                return result; // TODO: When C++20 comes with std::ranges, use a transform_view
                            },
                            [](const Interface *interface) { return interface->target(); }, // Get target of edge
                            [goal = to->source()](const Interface *interface) {
                                return goal == interface->target(); // Accept if we found the source of to
                            },
                            [](const Interface *interface) { // Don't use the null router.
                                return interface->target()->is_null();
                            }, cost_fn);
        if (!val) return false;
        auto elem = val.value();
        std::vector<Interface*> path{to, elem.edge};
        for (auto p = elem.back_pointer; p != nullptr; p = p->back_pointer) {
            path.push_back(p->edge);
        }
        std::reverse(path.begin(), path.end());
        return make_data_flow(from, path, next_label);
    }

}
