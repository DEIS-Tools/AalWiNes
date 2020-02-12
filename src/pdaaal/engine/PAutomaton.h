//
// Created by Morten on 22-01-2020.
//

#ifndef PDAAAL_PAUTOMATON_H
#define PDAAAL_PAUTOMATON_H

#include "UntypedPAutomaton.h"

namespace pdaaal {

    template<typename T>
    class PAutomaton : public UntypedPAutomaton {
    public:
        using tracestate_t = typename TypedPDA<T>::tracestate_t;

        // Accept one control state with given stack.
        PAutomaton(const TypedPDA<T> &pda, size_t initial_state, const std::vector<T> &initial_stack) : _typed_pda(pda) {
            const size_t size = pda.states().size();
            const size_t accepting = initial_stack.empty() ? initial_state : size;
            for (size_t i = 0; i < size; ++i) {
                add_state(true, i == accepting);
            }
            auto labels = pda.encode_pre(initial_stack);
            auto last_state = initial_state;
            for (size_t i = 0; i < labels.size(); ++i) {
                auto state = add_state(false, i == labels.size() - 1);
                add_edge(last_state, state, labels[i]);
                last_state = state;
            }
        };

        // Accept one control state with given stack.
        PAutomaton(const TypedPDA<T> &pda, size_t initial_state, const std::vector<uint32_t> &initial_stack) : _typed_pda(pda) {
            const size_t size = pda.states().size();
            const size_t accepting = initial_stack.empty() ? initial_state : size;
            for (size_t i = 0; i < size; ++i) {
                add_state(true, i == accepting);
            }
            auto last_state = initial_state;
            for (size_t i = 0; i < initial_stack.size(); ++i) {
                auto state = add_state(false, i == initial_stack.size() - 1);
                add_edge(last_state, state, initial_stack[i]);
                last_state = state;
            }
        };

        /* Not used. Keep it in case we need it anyway
        // Accept one control state with any one-element stack.
        PAutomaton(const TypedPDA<T> &pda, size_t state) : _typed_pda(pda) {
            const size_t size = pda.states().size();
            for (size_t i = 0; i < size; ++i) {
                add_state(true, false);
            }
            auto new_state = add_state(false, true);
            add_wildcard(state, new_state);
        };

        static PAutomaton<T> any_stack(const TypedPDA<T> &pda, size_t state) {
            PAutomaton<T> result(pda);
            const size_t size = pda.states().size();
            for (size_t i = 0; i < size; ++i) {
                result.add_state(true, i == state);
            }
            // Accept any stack, but avoid transitions into states in pda.
            auto new_state = result.add_state(false, true);
            result.add_wildcard(state, new_state);
            result.add_wildcard(new_state, new_state);
            return result;
        }
        */

        PAutomaton(PAutomaton<T> &&) noexcept = default;
        PAutomaton<T> &operator=(PAutomaton<T> &&) noexcept = default;
        PAutomaton(const PAutomaton<T> &other) : UntypedPAutomaton(other), _typed_pda(other._typed_pda) {};
        PAutomaton<T> &operator=(const PAutomaton<T> &other) {
            UntypedPAutomaton::operator=(other);
            _typed_pda = other._typed_pda;
            return *this;
        };

        virtual ~PAutomaton() = default;

        // const version of _pre_star()
        PAutomaton<T> pre_star() const {
            PAutomaton<T> result(*this);
            result._pre_star();
            return result;
        }

        // const version of _post_star()
        PAutomaton<T> post_star() const {
            PAutomaton<T> result(*this);
            result._post_star();
            return result;
        }

        std::vector<tracestate_t> get_trace(size_t state, const std::vector<T>& labels) const {
            assert(state < pda().states().size());
            auto stack = encode(labels);
            auto path = _accept_path(state, stack);
            return get_trace(path, stack);
        }
        std::vector<tracestate_t> _get_trace(size_t state, const std::vector<uint32_t>& stack) const {
            assert(state < pda().states().size());
            auto path = _accept_path(state, stack);
            return get_trace(path, stack);
        }

        void to_dot_with_symbols(std::ostream &out) const {
            to_dot(out, [this](auto &s, auto &e) { s << get_symbol(e._label); });
        }

        bool accepts(size_t state, const std::vector<T>& stack) {
            auto labels = encode(stack);
            return _accepts(state, labels);
        }

        std::vector<size_t> accept_path(size_t state, const std::vector<T> &labels) {
            auto stack = encode(labels);
            return _accept_path(state, stack);
        }

        const TypedPDA<T> &typed_pda() const { return _typed_pda; }

    protected:
        explicit PAutomaton(const TypedPDA<T> &pda) : _typed_pda(pda) {};

        [[nodiscard]] const PDA &pda() const override { return _typed_pda; }

        [[nodiscard]] size_t number_of_labels() const override { return _typed_pda.number_of_labels(); };

        T get_symbol(uint32_t i) const {
            return _typed_pda.get_symbol(i);
        };

        std::vector<uint32_t> encode(const std::vector<T>& labels) const {
            return _typed_pda.encode_pre(labels);
        }
        std::vector<tracestate_t> get_trace(const std::vector<size_t>& path, const std::vector<uint32_t>& stack) const;


    private:
        const TypedPDA<T> &_typed_pda;
    };

    template <typename T>
    std::vector<typename PAutomaton<T>::tracestate_t> PAutomaton<T>::get_trace(const std::vector<size_t>& path, const std::vector<uint32_t>& stack) const {
        if (path.empty()) {
            return std::vector<tracestate_t>();
        }
        std::deque<std::tuple<size_t, uint32_t, size_t>> edges;
        for (size_t i = stack.size(); i > 0; --i) {
            edges.emplace_back(path[i - 1], stack[i - 1], path[i]);
        }

        auto decode_edges = [this](const std::deque<std::tuple<size_t, uint32_t, size_t>> &edges) -> tracestate_t {
            tracestate_t result{std::get<0>(edges.back()), std::vector<T>()};
            auto num_labels = number_of_labels();
            for (auto it = edges.crbegin(); it != edges.crend(); ++it) {
                auto label = std::get<1>(*it);
                if (label < num_labels){
                    result._stack.emplace_back(get_symbol(label));
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
            const trace_t *trace_label = get_trace_label(edge);
            if (trace_label == nullptr) break;

            auto from = std::get<0>(edge);
            auto label = std::get<1>(edge);
            auto to = std::get<2>(edge);

            if (trace_label->is_pre_trace()) {
                // pre* trace
                auto &rule = _typed_pda.states()[from]._rules[trace_label->_rule_id];
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
                auto &rule = _typed_pda.states()[trace_label->_state]._rules[trace_label->_rule_id];
                switch (rule._operation) {
                    case PDA::POP:
                    case PDA::SWAP:
                    case PDA::NOOP:
                        edges.emplace_back(trace_label->_state, trace_label->_label, to);
                        break;
                    case PDA::PUSH:
                        auto &edge2 = edges.back();
                        edges.pop_back();
                        auto trace_label2 = get_trace_label(edge2);
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

#endif //PDAAAL_PAUTOMATON_H
