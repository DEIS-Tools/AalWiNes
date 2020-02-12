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
 * File:   Solver.h
 * Author: Morten K. Schou <morten@h-schou.dk>
 *
 * Created on 12-02-2020.
 */

#ifndef SOLVER_H
#define SOLVER_H

#include "PAutomaton.h"
#include "pdaaal/model/TypedPDA.h"

namespace pdaaal {

    class Solver {
    public:
        Solver() = default;

        virtual ~Solver() = default;

        using trace_info = typename std::pair<std::unique_ptr<PAutomaton>, size_t>;
        using res_type = typename std::pair<bool, trace_info>;

        template<typename T>
        res_type pre_star(const TypedPDA<T>& pda, bool build_trace) {
            auto automaton = std::make_unique<PAutomaton>(pda, pda.terminal(), pda.initial_stack());
            automaton->pre_star(); // automaton->pre_star(build_trace); // TODO: implement no-trace version.
            bool result = automaton->accepts(pda.initial(), pda.initial_stack());
            return std::make_pair(result, std::make_pair(std::move(automaton), pda.initial()));
        }

        template<typename T>
        res_type post_star(const TypedPDA<T>& pda, bool build_trace) {
            auto automaton = std::make_unique<PAutomaton>(pda, pda.initial(), pda.initial_stack());
            automaton->post_star(); // automaton->post_star(build_trace); // TODO: implement no-trace version.
            bool result = automaton->accepts(pda.terminal(), pda.initial_stack());
            return std::make_pair(result, std::make_pair(std::move(automaton), pda.terminal()));
        }

        template <typename T>
        [[nodiscard]] std::vector<typename TypedPDA<T>::tracestate_t> get_trace(const TypedPDA<T>& pda, trace_info info) const {
            auto stack = pda.initial_stack();
            auto path = info.first->accept_path(info.second, stack);
            auto trace = _get_trace(pda, info.first.get(), path, stack);
            trace.pop_back(); // Removes terminal state from trace.
            return trace;
        }

    private:
        template <typename T>
        std::vector<typename TypedPDA<T>::tracestate_t> _get_trace(const TypedPDA<T> &pda, const PAutomaton *automaton, const std::vector<size_t>& path, const std::vector<uint32_t>& stack) const;
    };

    template <typename T>
    std::vector<typename TypedPDA<T>::tracestate_t> Solver::_get_trace(const TypedPDA<T> &pda, const PAutomaton *automaton, const std::vector<size_t>& path, const std::vector<uint32_t>& stack) const {
        using tracestate_t = typename TypedPDA<T>::tracestate_t;

        if (path.empty()) {
            return std::vector<tracestate_t>();
        }
        std::deque<std::tuple<size_t, uint32_t, size_t>> edges;
        for (size_t i = stack.size(); i > 0; --i) {
            edges.emplace_back(path[i - 1], stack[i - 1], path[i]);
        }

        auto decode_edges = [&pda](const std::deque<std::tuple<size_t, uint32_t, size_t>> &edges) -> tracestate_t {
            tracestate_t result{std::get<0>(edges.back()), std::vector<T>()};
            auto num_labels = pda.number_of_labels();
            for (auto it = edges.crbegin(); it != edges.crend(); ++it) {
                auto label = std::get<1>(*it);
                if (label < num_labels){
                    result._stack.emplace_back(pda.get_symbol(label));
                }
            }
            return result;
        };

        bool post = false;

        std::vector<tracestate_t> trace;
        trace.push_back(decode_edges(edges));
        while (true) {
            auto &edge = edges.back();
            edges.pop_back();
            const trace_t *trace_label = automaton->get_trace_label(edge);
            if (trace_label == nullptr) break;

            auto from = std::get<0>(edge);
            auto label = std::get<1>(edge);
            auto to = std::get<2>(edge);

            if (trace_label->is_pre_trace()) {
                // pre* trace
                auto &rule = automaton->pda().states()[from]._rules[trace_label->_rule_id];
                switch (rule._operation) {
                    case PDA::POP:
                        break;
                    case PDA::SWAP:
                        edges.emplace_back(rule._to, rule._op_label, to);
                        break;
                    case PDA::NOOP:
                        edges.emplace_back(rule._to, label, to);
                        break;
                    case PDA::PUSH:
                        edges.emplace_back(trace_label->_state, label, to);
                        edges.emplace_back(rule._to, rule._op_label, trace_label->_state);
                        break;
                }
                trace.push_back(decode_edges(edges));

            } else if (trace_label->is_post_epsilon_trace()) {
                // Intermediate post* trace
                // Current edge is the result of merging with an epsilon edge.
                // Reconstruct epsilon edge and the other edge.
                edges.emplace_back(trace_label->_state, label, to);
                edges.emplace_back(from, std::numeric_limits<uint32_t>::max(), trace_label->_state);

            } else {
                // post* trace
                auto &rule = automaton->pda().states()[trace_label->_state]._rules[trace_label->_rule_id];
                switch (rule._operation) {
                    case PDA::POP:
                    case PDA::SWAP:
                    case PDA::NOOP:
                        edges.emplace_back(trace_label->_state, trace_label->_label, to);
                        break;
                    case PDA::PUSH:
                        auto &edge2 = edges.back();
                        edges.pop_back();
                        auto trace_label2 = automaton->get_trace_label(edge2);
                        edges.emplace_back(trace_label2->_state, trace_label2->_label, std::get<2>(edge2));
                        break;
                }
                trace.push_back(decode_edges(edges));
                post = true;
            }
        }
        if (post) {
            std::reverse(trace.begin(), trace.end());
        }
        return trace;
    }
}

#endif /* SOLVER_H */

