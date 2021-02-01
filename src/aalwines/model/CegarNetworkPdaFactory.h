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
 *  Copyright Peter G. Jensen and Morten K. Schou
 */

/* 
 * File:   CegarNetworkPdaFactory.h
 * Author: Morten K. Schou <morten@h-schou.dk>
 *
 * Created on 03-12-2020.
 */

#ifndef AALWINES_CEGARNETWORKPDAFACTORY_H
#define AALWINES_CEGARNETWORKPDAFACTORY_H

#include <aalwines/model/Query.h>
#include <aalwines/model/Network.h>
#include <aalwines/query/QueryBuilder.h>
#include <pdaaal/CegarPdaFactory.h>

#include <utility>

namespace aalwines {

    struct State {
        using nfa_state_t = typename pdaaal::NFA<Query::label_t>::state_t;
        bool _initial = false;
        size_t _eid = 0; // which entry we are going for
        size_t _rid = 0; // which rule in that entry
        size_t _opid = std::numeric_limits<size_t>::max(); // which operation is the first in the rule (max = no operations left).
        const Interface *_inf = nullptr;
        const nfa_state_t* _nfa_state = nullptr;
        State() = default;
        State(size_t eid, size_t rid, size_t opid, const Interface* inf, const nfa_state_t* nfa_state)
        : _eid(eid), _rid(rid), _opid(opid), _inf(inf), _nfa_state(nfa_state) { };
        State(const Interface* inf, const nfa_state_t* nfa_state, bool initial = false)
                : _initial(initial), _inf(inf), _nfa_state(nfa_state) { };
        bool operator==(const State& other) const {
            return _initial == other._initial && _eid == other._eid && _rid == other._rid &&
                   _opid == other._opid && _inf == other._inf && _nfa_state == other._nfa_state;
        }
        bool operator!=(const State& other) const {
            return !(*this == other);
        }
        [[nodiscard]] const Interface* interface() const {
            return _inf;
        }
        [[nodiscard]] bool ops_done() const {
            return _opid == std::numeric_limits<size_t>::max();
        }
        [[nodiscard]] bool accepting() const {
            return ops_done() && (_inf == nullptr || !_inf->is_virtual()) && _nfa_state->_accepting;
        }
        static State perform_op(const State& state) {
            assert(!state.ops_done());
            return perform_op(state, state._inf->table().entries()[state._eid]._rules[state._rid]);
        }
        static State perform_op(const State& state, const RoutingTable::forward_t& forward) {
            assert(!state.ops_done());
            if (state._opid + 2 == forward._ops.size()) {
                return State(forward._via->match(), state._nfa_state);
            } else {
                return State(state._eid, state._rid, state._opid + 1, state._inf, state._nfa_state);
            }
        }
    } __attribute__((packed)); // packed is needed to make this work fast with ptries

    class StateMapping {
    protected:
        using state_t = State;
    public:
        [[nodiscard]] size_t map_id(size_t concrete_id) const {
            return _state_map.get_data(concrete_id); // abstract_id
        }
        [[nodiscard]] state_t get_concrete_value(size_t concrete_id) const {
            return _state_map.at(concrete_id);
        }
        [[nodiscard]] bool match_concrete(state_t concrete_value, size_t concrete_id) const {
            auto [exists, res_id] = _state_map.exists(concrete_value);
            return exists && res_id == concrete_id;
        }
        [[nodiscard]] bool match_abstract(state_t concrete_value, size_t abstract_id) const {
            auto [exists, concrete_id] = _state_map.exists(concrete_value);
            return exists && _state_map.get_data(concrete_id) == abstract_id;
        }
        [[nodiscard]] size_t size() const {
            return _state_map.size();
        }

    protected:
        pdaaal::ptrie_map<state_t, size_t> _state_map;
    };

    class StateAbstraction : public StateMapping {
        using label_t = Query::label_t;
        using nfa_state_t = typename pdaaal::NFA<label_t>::state_t;
        using abstract_state_t = std::tuple<size_t, const nfa_state_t*, std::vector<std::tuple<RoutingTable::op_t, uint32_t>>>;
    public:
        StateAbstraction(std::function<std::pair<bool,size_t>(const label_t&)>&& abstract_label_lookup,
                         std::function<size_t(const Interface*)>&& interface_abstraction_fn)
        : _abstract_label_lookup(std::move(abstract_label_lookup)),
          _interface_abstraction_fn(std::move(interface_abstraction_fn)) {};

        // concrete_fresh, concrete_id, abstract_fresh, abstract_id
        std::tuple<bool,size_t,bool,size_t> insert(const state_t& state) {
            auto [concrete_fresh, concrete_id] = _state_map.insert(state);
            if (!concrete_fresh) return {false, concrete_id, false, _state_map.get_data(concrete_id)};
            auto [abstract_fresh, abstract_id] = _abstract_states.insert(abstract_state(state));
            _state_map.get_data(concrete_id) = abstract_id;
            return {true, concrete_id, abstract_fresh, abstract_id};
        }

        [[nodiscard]] size_t size_abstract() const {
            return _abstract_states.size();
        }

    private:
        abstract_state_t abstract_state(const state_t& state) {
            const Interface* interface = state.ops_done() ? state._inf : state._inf->table().entries()[state._eid]._rules[state._rid]._via->match();
            abstract_state_t result{
                    _interface_abstraction_fn(interface),
                    state._nfa_state, // No change here.
                    std::vector<std::tuple<RoutingTable::op_t, uint32_t>>()};
            if (!state.ops_done()) {
                const auto& ops = state._inf->table().entries()[state._eid]._rules[state._rid]._ops;
                assert(state._opid + 1 < ops.size());
                for (size_t i = state._opid + 1; i < ops.size(); ++i) {
                    assert(ops[i]._op == RoutingTable::op_t::POP || _abstract_label_lookup(ops[i]._op_label).first);
                    std::get<2>(result).emplace_back(ops[i]._op,
                                                     (ops[i]._op == RoutingTable::op_t::POP)
                                                      ? std::numeric_limits<uint32_t>::max()
                                                      : _abstract_label_lookup(ops[i]._op_label).second);
                }
            }
            return result;
        }

        std::function<std::pair<bool,size_t>(const label_t&)> _abstract_label_lookup;
        std::function<size_t(const Interface*)> _interface_abstraction_fn;
        pdaaal::ptrie_set<abstract_state_t> _abstract_states;
    };

    struct Rule {
        using label_t = Query::label_t;

        size_t _from_id; // concrete_id for from stat. Through a _state_map this gives us the interface
        size_t _eid; // entry id
        size_t _rid; // forwarding rule id in that entry
        size_t _to_id;

        Rule() = default;
        Rule(size_t from_id, size_t eid, size_t rid, size_t to_id)
        : _from_id(from_id), _eid(eid), _rid(rid), _to_id(to_id) {};

        [[nodiscard]] std::tuple<label_t,pdaaal::op_t,label_t> get(const StateMapping& state_map) const {
            const auto& e = entry(state_map);
            auto [op, op_label] = first_action(e);
            return {e._top_label, op, op_label};
        }
        [[nodiscard]] const RoutingTable::entry_t& entry(const StateMapping& state_map) const {
            return state_map.get_concrete_value(_from_id)._inf->table().entries()[_eid];
        }
        [[nodiscard]] const RoutingTable::forward_t& forward(const StateMapping& state_map) const {
            return forward(entry(state_map));
        }
        [[nodiscard]] const RoutingTable::forward_t& forward(const RoutingTable::entry_t& entry) const {
            return entry._rules[_rid];
        }
        [[nodiscard]] std::pair<pdaaal::op_t,label_t> first_action(const StateMapping& state_map) const {
            return first_action(forward(state_map));
        }
        [[nodiscard]] std::pair<pdaaal::op_t,label_t> first_action(const RoutingTable::entry_t& entry) const {
            return first_action(forward(entry));
        }
        static std::pair<pdaaal::op_t,label_t> first_action(const RoutingTable::forward_t& forward) {
            if (forward._ops.empty()) {
                return std::make_pair(pdaaal::NOOP, label_t());
            } else {
                return convert_action(forward._ops[0]);
            }
        }
        static std::pair<pdaaal::op_t,label_t> convert_action(const RoutingTable::action_t& action) {
            pdaaal::op_t op;
            label_t op_label;
            switch (action._op) {
                case RoutingTable::op_t::POP:
                    op = pdaaal::POP;
                    break;
                case RoutingTable::op_t::PUSH:
                    op = pdaaal::PUSH;
                    op_label = action._op_label;
                    break;
                case RoutingTable::op_t::SWAP:
                    op = pdaaal::SWAP;
                    op_label = action._op_label;
                    break;
            }
            return std::make_pair(op, op_label);
        }
    };

/*
    class RuleAbstraction {
        // It was a bit hard to use AbstractionMappings directly for this, so just make custom one for now.
    public:
        using label_t = Query::label_t;
        using abstract_label_t = uint32_t;
        using nfa_state_t = typename pdaaal::NFA<Query::label_t>::state_t;
        using state_t = State;
        using abstract_state_t = std::tuple<size_t, const nfa_state_t*, std::vector<std::tuple<RoutingTable::op_t, uint32_t>>>;
        using rule_t = std::tuple<size_t, label_t, size_t, pdaaal::op_t, label_t>; // From and to state is concrete_id (size_t), not abstract_id!
        using abstract_rule_t = pdaaal::user_rule_t<void,std::less<void>>;

        RuleAbstraction() : _rule_abstraction([this](const rule_t& rule){ return this->abstract_rule(rule); }) {};
        RuleAbstraction(std::function<std::pair<bool,size_t>(const label_t&)>&& abstract_label_lookup)
        : _abstract_label_lookup(std::move(abstract_label_lookup)),
          _rule_abstraction([this](const rule_t& rule){ return this->abstract_rule(rule); }) {};

        std::pair<bool,size_t> insert(const rule_t& rule) {
            return _rule_abstraction.insert(rule);
        }
        std::pair<bool,size_t> insert(size_t from, const label_t& pre, size_t to, pdaaal::op_t op, const label_t& op_label) {
            return _rule_abstraction.insert(rule_t(from, pre, to, op, op_label));
        }
        [[nodiscard]] size_t size() const {
            return _rule_abstraction.size();
        }
        [[nodiscard]] abstract_rule_t get_abstract_value(size_t id) const {
            return _rule_abstraction.get_abstract_value(id);
        }

    private:
        abstract_rule_t abstract_rule(const rule_t& rule) {
            const auto& [from, pre, to, op, op_label] = rule;
            abstract_rule_t result;
            result._from = _state_abstraction.map_id(from);
            assert(_abstract_label_lookup(pre).first);
            result._pre = _abstract_label_lookup(pre).second;
            result._to = _state_abstraction.map_id(from);
            result._op = op;
            assert(!(op == pdaaal::PUSH || op == pdaaal::SWAP) || _abstract_label_lookup(op_label).first);
            result._op_label = (op == pdaaal::PUSH || op == pdaaal::SWAP)
                               ? _abstract_label_lookup(op_label).second
                               : std::numeric_limits<uint32_t>::max();
        }

    private:
        std::function<std::pair<bool,size_t>(const label_t&)> _abstract_label_lookup;
        StateAbstraction _state_abstraction;
        pdaaal::AbstractionMapping<rule_t, abstract_rule_t> _rule_abstraction;
    };*/



    template <typename W_FN, typename W, typename C, typename A> class CegarNetworkPdaReconstruction;

    // FIXME: For now simple unweighted version.
    template<typename W_FN = std::function<void(void)>, typename W = typename W_FN::result_type, typename C = std::less<W>, typename A = pdaaal::add<W>>
    class CegarNetworkPdaFactory : public pdaaal::CegarPdaFactory<Query::label_t, W, C, A> {
        friend class CegarNetworkPdaReconstruction<W_FN,W,C,A>;
    public:
        using label_t = Query::label_t;
    private:
        using state_t = State;
        using NFA = pdaaal::NFA<label_t>; // TODO: Make link NFA different type from header NFA. (low priority...)
        using nfa_state_t = typename NFA::state_t;
        using abstract_state_t = std::tuple<size_t, const nfa_state_t*, std::vector<std::tuple<RoutingTable::op_t, uint32_t>>>;
        using parent_t = pdaaal::CegarPdaFactory<label_t, W, C, A>;
        using abstract_rule_t = typename parent_t::abstract_rule_t;
        using rule_t = Rule; //std::tuple<size_t, label_t, size_t, pdaaal::op_t, label_t>; // From and to state is concrete_id (size_t), not abstract_id!
    public:

        template<typename label_abstraction_fn_t>
        CegarNetworkPdaFactory(const Network &network, const NFA& path, size_t failures,
                               std::unordered_set<label_t>&& all_labels,
                               label_abstraction_fn_t&& label_abstraction_fn,
                               std::function<size_t(const Interface*)>&& interface_abstraction_fn)
        : parent_t(all_labels, std::forward<label_abstraction_fn_t>(label_abstraction_fn)),
          _all_labels(std::move(all_labels)), _network(network), _path(path), _failures(failures),
          _rule_mapping([this](const rule_t& rule){ return this->abstract_rule(rule, _state_mapping); }) /* use of _state_mapping here is just dummy, overwritten in initialize(). */
        {
            static_assert(std::is_convertible_v<label_abstraction_fn_t,
                    std::function<decltype(std::declval<label_abstraction_fn_t>()(std::declval<const label_t&>()))(const label_t&)>>);
            std::cout << std::endl; // FIXME: Remove...
            initialize(std::move(interface_abstraction_fn));
        }

        // Forward declaration of refinement constructors
        void refine(std::pair<Refinement<const Interface*>, Refinement<label_t>>&& refinement) {
            _interface_abstraction.refine(refinement.first);
            // TODO: Optimization: Find a way to refine and reuse states and rules...
            reinitialize();

        }
        void refine(HeaderRefinement<label_t>&& refinement) {
            // TODO: Optimization: Find a way to refine and reuse states and rules...
            reinitialize();
        }

    protected:

        void build_pda() override {
            for (size_t i = 0; i < _rule_mapping.size(); ++i) {
                this->add_rule(_rule_mapping.get_abstract_value(i));
            }
            std::cout << "; rules: " << _rule_mapping.size() << "; labels: " << this->number_of_labels() << "; (interfaces: " << _interface_abstraction.size() << ")" << std::endl; // FIXME: Remove...
        }
        const std::vector<size_t>& initial() override {
            return _initial;
        }
        const std::vector<size_t>& accepting() override {
            return _accepting;
        }

    private:
        void initialize(std::function<size_t(const Interface*)>&& interface_abstraction_fn) {
            // First time we populate interface_abstraction
            pdaaal::AbstractionMapping<const Interface*,size_t> interface_abstraction(std::move(interface_abstraction_fn));
            make_rules(StateAbstraction(
                    [this](const label_t& label) { return this->abstract_label(label); },
                    [&interface_abstraction](const Interface* i){ return interface_abstraction.insert(i).second; })
            );
            _interface_abstraction = pdaaal::RefinementMapping<const Interface*>(std::move(interface_abstraction));
        }
        void reinitialize() {
            // After first time, we (refine and) reuse interface_abstracion, but recreate all (refined) states and rules.
            // TODO: Optimization: Find a way to refine and reuse states and rules...
            _initial.clear();
            _accepting.clear();
            make_rules(StateAbstraction(
                    [this](const label_t& label) { return this->abstract_label(label); },
                    [this](const Interface* i){
                        assert(_interface_abstraction.exists(i).first);
                        return _interface_abstraction.exists(i).second; })
            );
        }
        void make_rules(StateAbstraction&& state_abstraction) {
            _rule_mapping = pdaaal::AbstractionMapping<rule_t,abstract_rule_t>(
                    [this,&state_abstraction](const rule_t& rule){ return this->abstract_rule(rule, state_abstraction); });

            // TODO: Add support for k>0.
            if (_failures > 0) throw base_error("CEGAR for k>0 not yet implemented.");

            std::vector<std::pair<State,size_t>> waiting;
            auto add_state = [&waiting,&state_abstraction,this](State&& state){
                auto [concrete_fresh, concrete_id, abstract_fresh, abstract_id] = state_abstraction.insert(state);
                if (abstract_fresh) {
                    if (state._initial) {
                        _initial.push_back(abstract_id);
                    }
                    if (state.accepting()) {
                        _accepting.push_back(abstract_id);
                    }
                }
                if (concrete_fresh) {
                    waiting.emplace_back(state, concrete_id);
                }
                return concrete_id;
            };

            auto add_initial = [&add_state, this](const std::vector<nfa_state_t*>& next, const Interface* inf) {
                if (inf != nullptr && inf->is_virtual()) return; // don't start on a virtual interface.
                for (const auto& n : next) {
                    add_state(State(inf, n, true));
                }
            };

            // First make initial states.
            for (const auto& i : _path.initial()) {
                for (const auto& e : i->_edges) {
                    auto next = e.follow_epsilon();
                    if (e.wildcard(_network.all_interfaces().size())) {
                        for (const auto& inf : _network.all_interfaces()) {
                            add_initial(next, inf->match());
                        }
                    } else if (!e._negated) {
                        for (const auto& s : e._symbols) {
                            assert(s.type() == Query::INTERFACE);
                            auto inf = _network.all_interfaces()[s.value()];
                            add_initial(next, inf->match());
                        }
                    } else {
                        for (const auto& inf : _network.all_interfaces()) {
                            auto iid = Query::label_t{Query::INTERFACE, 0, inf->global_id()};
                            auto lb = std::lower_bound(e._symbols.begin(), e._symbols.end(), iid);
                            assert(std::is_sorted(e._symbols.begin(), e._symbols.end()));
                            if (lb == std::end(e._symbols) || *lb != iid) {
                                add_initial(next, inf->match());
                            }
                        }
                    }
                }
            }

            // Now search through states, adding rules and new states.
            while (!waiting.empty()) {
                auto [from_state,from_id] = waiting.back(); // Concrete state and id
                waiting.pop_back();

                if (from_state.ops_done()) { // No ops left to do, so we generate rules for the routing table at from_state._inf
                    const auto& entries = from_state._inf->table().entries();
                    for (size_t eid = 0; eid < entries.size(); ++eid) {
                        const auto& entry = entries[eid];
                        for (size_t rid = 0; rid < entry._rules.size(); ++rid) {
                            const auto& forward = entry._rules[rid];
                            if (forward._priority > 0) continue; // TODO: This only works for k==0 !!!
                            auto apply_per_nfastate = [&](const nfa_state_t* n) {
                                size_t to_id = (forward._ops.size() <= 1)
                                               ? add_state(State(forward._via->match(), n))
                                               : add_state(State(eid, rid, 0, from_state._inf, n));
                                _rule_mapping.insert(rule_t(from_id, eid, rid, to_id));
                            };
                            if (forward._via->is_virtual()) { // Virtual interface does not use a link, so keep same NFA state.
                                apply_per_nfastate(from_state._nfa_state);
                            } else { // Follow NFA edges matching forward._via
                                for (const auto& e : from_state._nfa_state->_edges) {
                                    if (!e.contains(Query::label_t{Query::INTERFACE, 0, forward._via->global_id()})) continue;
                                    for (const auto& n : e.follow_epsilon()) {
                                        apply_per_nfastate(n);
                                    }
                                }
                            }

                        }
                    }
                } else {
                    const auto& entry = from_state._inf->table().entries()[from_state._eid];
                    const auto& forward = entry._rules[from_state._rid];
                    assert(from_state._opid + 1 < forward._ops.size()); // Otherwise we would already have moved to the next interface.
                    const auto& action = forward._ops[from_state._opid + 1];

                    auto pre = forward._ops[from_state._opid]._op_label;
                    if (forward._ops[from_state._opid]._op != RoutingTable::op_t::SWAP &&
                        forward._ops[from_state._opid]._op != RoutingTable::op_t::PUSH) {
                        throw base_error("Unexpected pop!"); // TODO: This is not intended behaviour, but we need wildcard label in PDA to handle it properly.!
                        assert(false);
                    }
                    auto [op, op_label] = Rule::convert_action(action);
                    size_t to_id = add_state(State::perform_op(from_state, forward));
                    _rule_mapping.insert_abstract(abstract_rule_t(
                        state_abstraction.map_id(from_id),
                        this->abstract_label(pre).second,
                        state_abstraction.map_id(to_id),
                        op,
                        (op == pdaaal::PUSH || op == pdaaal::SWAP) ? this->abstract_label(op_label).second : std::numeric_limits<uint32_t>::max())
                    );
                }
            }

            std::sort(_initial.begin(), _initial.end());
            std::sort(_accepting.begin(), _accepting.end());

            std::cout << "states: " << state_abstraction.size_abstract(); // FIXME: Remove...
            _state_mapping = std::move(state_abstraction);
        }

        /*std::vector<label_t> expand_pre_label(const label_t& pre) {
            auto val = pre.value();
            auto mask = pre.mask();
            std::vector<label_t> result;

            switch (pre.type()) {
                case Query::STICKY_MPLS:
                case Query::MPLS:
                    assert(mask == 0);
                    result.push_back(pre);
                    break;
                case Query::ANYIP:
                    mask = 64;
                    val = 0;
                    // fall through to IP
                case Query::IP4:
                    for (auto &l : Builder::get_labels(this->_all_labels, val, mask, Query::IP4)) {
                        assert(l.mask() == 0 || l.type() != Query::IP4 || l == Query::label_t::any_ip4);
                        result.push_back(l);
                    }
                    if (pre.type() != Query::ANYIP) break; // fall through to IP6 if any
                case Query::IP6:
                    for (auto &l : Builder::get_labels(this->_all_labels, val, mask, Query::IP6)) {
                        assert(l.mask() == 0 || l.type() != Query::IP6 || l == Query::label_t::any_ip6);
                        result.push_back(l);
                    }
                    break;
                default:
                    throw base_error("Unknown label-type");
            }
            return result;
        }*/

        abstract_rule_t abstract_rule(const rule_t& rule, const StateMapping& state_mapping) {
            abstract_rule_t result;
            result._from = state_mapping.map_id(rule._from_id);
            const auto& entry = rule.entry(state_mapping);
            const auto& forward = rule.forward(entry);
            auto pre = entry._top_label;
            auto [op, op_label] = rule_t::first_action(forward);
            assert(this->abstract_label(pre).first);
            result._pre = this->abstract_label(pre).second;
            result._to = state_mapping.map_id(rule._to_id);
            result._op = op;
            assert(!(op == pdaaal::PUSH || op == pdaaal::SWAP) || this->abstract_label(op_label).first);
            result._op_label = (op == pdaaal::PUSH || op == pdaaal::SWAP)
                               ? this->abstract_label(op_label).second
                               : std::numeric_limits<uint32_t>::max();
            //if constexpr (is_weighted) { // FIXME: Weights...
            //    result._weight = _weight_f(forward, entry);
            //}
            return result;
        }

    private:
        std::unordered_set<label_t> _all_labels;
        const Network& _network;
        const NFA& _path;
        const size_t _failures;
        pdaaal::RefinementMapping<const Interface*> _interface_abstraction; // This is what gets refined by CEGAR.
        StateMapping _state_mapping;
        //pdaaal::RefinementMapping<State> _state_abstraction; // This needs to be refined based on the refinement of _interface_abstraction and labels... // TODO: How?!?
        pdaaal::AbstractionMapping<rule_t,abstract_rule_t> _rule_mapping;
        std::vector<size_t> _initial;
        std::vector<size_t> _accepting;
    };


    struct ConfigurationRange {
    public:
        using value_type = std::tuple<pdaaal::Header<Query::label_t>, State, State, size_t, size_t>;
    private:
        using rule_t = Rule;
        using rule_range_t = pdaaal::RefinementMapping<rule_t>::concrete_value_range;
        using transform_fn = std::function<std::optional<value_type>(const rule_t&)>;
    public:
        struct sentinel;
        struct iterator {
            using rule_iterator = rule_range_t::iterator;
        private:
            rule_iterator _rule_it;
            rule_iterator _rule_end;
            const transform_fn& _transform;
            value_type _current;
            bool _singleton = false;

            iterator(const rule_iterator& rule_it, const rule_iterator& rule_end, const transform_fn& transform) noexcept
            : _rule_it(rule_it), _rule_end(rule_end), _transform(transform) {
                while(!(is_end() || is_valid())) { // Go forward to the first valid element (or the end).
                    ++_rule_it;
                }
            };
        public:
            iterator(const rule_range_t& rule_range, const transform_fn& transform) noexcept
            : iterator(rule_range.begin(), rule_range.end(), transform) { };

            iterator(const value_type& elem, const rule_iterator& rule_it, const rule_iterator& rule_end, const transform_fn& transform)
            : _rule_it(rule_it), _rule_end(rule_end), _transform(transform), _current(elem), _singleton(true) { };

            value_type operator*() const {
                return _current; // We have already computed it when checking is_valid, so we use the stored value.
            }
            iterator& operator++() noexcept {
                if (_singleton) {
                    _rule_it = _rule_end;
                } else {
                    do {
                        ++_rule_it;
                    } while(!(is_end() || is_valid()));
                }
                return *this;
            }
            //iterator operator++(int) noexcept { return iterator(_inner++, _state_abstraction); }
            bool operator==(const sentinel& rhs) const noexcept { return is_end(); }
            bool operator!=(const sentinel& rhs) const noexcept { return !(*this == rhs); }
        private:
            bool is_valid() {
                std::optional<value_type> res = _transform(*_rule_it);
                if (!res) return false;
                _current = *std::move(res);
                return true;
            }
            [[nodiscard]] bool is_end() const {
                return _rule_it == _rule_end;
            }
        };
        struct sentinel {
            bool operator==(const iterator& it) const noexcept { return it == *this; }
            bool operator!=(const iterator& it) const noexcept { return !(it == *this); }
        };
        using const_iterator = iterator;

        ConfigurationRange(rule_range_t&& rules, transform_fn&& transform)
        : _rules(std::move(rules)), _transform(std::move(transform)) {};

        explicit ConfigurationRange(const value_type& elem) : _elem(elem), _singleton(true) { };
        explicit ConfigurationRange(value_type&& elem) : _elem(std::move(elem)), _singleton(true) { };

        [[nodiscard]] iterator begin() const {
            return _singleton ? iterator(_elem, _rules.begin(), _rules.end(), _transform) : iterator(_rules, _transform);
        }
        [[nodiscard]] sentinel end() const {
            return sentinel();
        }

    private:
        // TODO: This support for combined singleton range and transform of rules range is quite ugly, and has some overhead.
        //  The singleton case could maybe be handled by PDAAAL by extending the CEGAR model interface.
        std::vector<size_t> _dummy_range{1}; // Default used by singleton...
        rule_range_t _rules = rule_range_t(nullptr, &_dummy_range);
        transform_fn _transform = [](const rule_t&){ return std::nullopt; };
        value_type _elem; // For singleton_range.
        bool _singleton = false;
    };


    // For now simple unweighted version.
    template<typename W_FN = std::function<void(void)>, typename W = typename W_FN::result_type, typename C = std::less<W>, typename A = pdaaal::add<W>>
    class CegarNetworkPdaReconstruction : public pdaaal::CegarPdaReconstruction<
            Query::label_t, // label_t
            const Interface*, // state_t
            ConfigurationRange,
            /*std::vector< // configuration_range_t
                    std::tuple<pdaaal::Header<Query::label_t>, // header_t
                            State,State,size_t,size_t> // old_state, state, eid, rid   (old_state._inf matches eid, rid).
            >,*/
            json , // concrete_trace_t
            W, C, A> {
        friend class CegarNetworkPdaFactory<W_FN,W,C,A>;
        using factory = CegarNetworkPdaFactory<W_FN,W,C,A>;
    public:
        using label_t = typename factory::label_t;
        using concrete_trace_t = json;
    private:
        using state_t = const Interface*; // This is the 'state' that is refined.
        using header_t = pdaaal::Header<Query::label_t>;
        //using configuration_t = std::tuple<header_t, State, State, size_t, size_t>; // header, old_state state, entry id, forwarding rule id
        //using configuration_range_t = std::vector<configuration_t>;
        using configuration_range_t = ConfigurationRange;
        using configuration_t = typename configuration_range_t::value_type;
        using parent_t = pdaaal::CegarPdaReconstruction<label_t, state_t, configuration_range_t, concrete_trace_t, W, C, A>;
        using abstract_rule_t = typename parent_t::abstract_rule_t;
        using rule_t = typename factory::rule_t;
        using solver_instance_t = typename parent_t::solver_instance_t;
    public:
        using refinement_t = typename parent_t::refinement_t;
        using header_refinement_t = typename parent_t::header_refinement_t;

        explicit CegarNetworkPdaReconstruction(const factory& factory, const solver_instance_t& instance, const pdaaal::NFA<label_t>& initial_headers, const pdaaal::NFA<label_t>& final_headers)
        : parent_t(instance, initial_headers, final_headers),
          _interface_abstraction(factory._interface_abstraction), _state_mapping(factory._state_mapping), _rule_mapping(factory._rule_mapping) {};

    protected:

        configuration_range_t initial_concrete_rules(const abstract_rule_t& abstract_rule) override {
            return ConfigurationRange(_rule_mapping.get_concrete_values_range(abstract_rule),
              [header=this->initial_header(),this](const rule_t& rule) -> std::optional<configuration_t> {
                auto from_state = _state_mapping.get_concrete_value(rule._from_id);
                if (!from_state._initial) return std::nullopt;
                return make_configuration(from_state, rule, header);
            });
        }
        refinement_t find_initial_refinement(const abstract_rule_t& abstract_rule) override {
            std::vector<std::pair<state_t,label_t>> X;

            auto labels = this->pre_labels(this->initial_header());
            for (size_t i = 0; i < _state_mapping.size(); ++i) { // TODO: Make this more efficient.
                if (_state_mapping.map_id(i) != abstract_rule._from) continue;
                auto state = _state_mapping.get_concrete_value(i);
                if (!state._initial) continue;
                for (const auto& label : labels) {
                    if (this->label_maps_to(label, abstract_rule._pre)) {
                        X.emplace_back(state.interface(), label);
                    }
                }
            }
            return find_refinement_common(X, abstract_rule);
        }
        configuration_range_t search_concrete_rules(const abstract_rule_t& abstract_rule, const configuration_t& conf) override {
            const auto& [header, old_state, state, eid, rid] = conf;
            if (state.ops_done()) {
                return ConfigurationRange(_rule_mapping.get_concrete_values_range(abstract_rule),
                  [&header,&state,this](const rule_t& rule) -> std::optional<configuration_t> {
                    if (!_state_mapping.match_concrete(state, rule._from_id)) return std::nullopt;
                    return make_configuration(state, rule, header);
                });
            } else {
                return ConfigurationRange({header, old_state, State::perform_op(state), eid, rid});
            }
        }
        refinement_t find_refinement(const abstract_rule_t& abstract_rule, const std::vector<configuration_t>& configurations) override {
            std::vector<std::pair<state_t,label_t>> X;

            for (const auto& [header, old_state, state, eid, rid] : configurations) {
                assert(state.ops_done());
                if (!_state_mapping.match_abstract(state, abstract_rule._from)) continue;
                for (const auto& label : this->pre_labels(header)) {
                    if (this->label_maps_to(label, abstract_rule._pre)) {
                        X.emplace_back(state._inf, label);
                    }
                }
            }
            return find_refinement_common(X, abstract_rule);
        }
        header_t get_header(const configuration_t& conf) override {
            return std::get<0>(conf);
        }
        void add_link_to_trace(json& trace, const State& state, const std::vector<label_t>& final_header) const {
            trace.emplace_back();
            trace.back()["from_router"] = state._inf->target()->name();
            trace.back()["from_interface"] = state._inf->match()->get_name();
            trace.back()["to_router"] = state._inf->source()->name();
            trace.back()["to_interface"] = state._inf->get_name();
            trace.back()["stack"] = json::array();
            std::for_each(final_header.rbegin(), final_header.rend(), [&stack=trace.back()["stack"]](const auto& label){
                std::stringstream s;
                s << label;
                stack.emplace_back(s.str());
            });
        }
        void add_rule_to_trace(json& trace, const Interface* inf, const RoutingTable::entry_t& entry, const RoutingTable::forward_t& rule) const {
            trace.emplace_back();
            trace.back()["ingoing"] = inf->get_name();
            std::stringstream s;
            s << entry._top_label;
            trace.back()["pre"] = s.str();
            trace.back()["rule"] = rule.to_json();
            /*if constexpr (is_weighted) { // TODO: Weighted...
                trace.back()["priority-weight"] = json::array();
                auto weights = _weight_f(rule, entry);
                for (const auto& w : weights){
                    trace.back()["priority-weight"].push_back(std::to_string(w));
                }
            }*/
        }
        concrete_trace_t get_concrete_trace(std::vector<configuration_t>&& configurations, std::vector<label_t>&& final_header, size_t initial_abstract_state) override {
            concrete_trace_t trace = json::array();
            for (auto it = configurations.crbegin(); it < configurations.crend(); ++it) {
                const auto& [header, from_state, state, eid, rid] = *it;
                if (!state.ops_done()) continue;
                add_link_to_trace(trace, state, final_header);
                const auto& entry = from_state._inf->table().entries()[eid];
                const auto& forward = entry._rules[rid];
                add_rule_to_trace(trace, from_state._inf, entry, forward);
                const auto& ops = forward._ops;
                // FIXME: This only works for operation sequences on the form:  POP | SWAP? PUSH*    (as checked by these asserts)
                assert(ops.size() <= 1 || !std::any_of(ops.begin(), ops.end(), [](const auto& op){ return op._op == RoutingTable::op_t::POP; }));
                assert(std::count_if(ops.begin(), ops.end(), [](const auto& op){ return op._op == RoutingTable::op_t::SWAP; }) <= ((ops.empty() || ops[0]._op == RoutingTable::op_t::SWAP) ? 1 : 0));
                for (auto op_it = ops.crbegin(); op_it < ops.crend(); ++op_it) {
                    switch (op_it->_op) {
                        case RoutingTable::op_t::POP:
                            final_header.push_back(entry._top_label);
                            break;
                        case RoutingTable::op_t::SWAP:
                            final_header.back() = entry._top_label;
                            break;
                        case RoutingTable::op_t::PUSH:
                            final_header.pop_back();
                            break;
                    }
                }
            }
            State first_state;
            if (configurations.empty()) {
                size_t concrete_id;
                for (concrete_id = 0; concrete_id < _state_mapping.size(); ++concrete_id) {
                    if (_state_mapping.map_id(concrete_id) == initial_abstract_state) break;
                }
                assert(concrete_id < _state_mapping.size());
                first_state = _state_mapping.get_concrete_value(concrete_id);
            } else {
                first_state = std::get<1>(configurations[0]);
            }
            add_link_to_trace(trace,first_state, final_header);
            std::reverse(trace.begin(), trace.end());
            return trace;
        }
    private:

        std::optional<configuration_t> make_configuration(const State& state, const rule_t& rule, const header_t& header) {
            auto [pre_label, op, op_label] = rule.get(_state_mapping);
            auto to_state = _state_mapping.get_concrete_value(rule._to_id);
            auto [additional_pops, post] = compute_pop_post(pre_label, op, op_label, to_state);
            auto new_header = this->update_header(header, pre_label, post, additional_pops);
            if (new_header) {
                return std::make_optional<configuration_t>(new_header.value(), state, to_state, rule._eid, rule._rid);
            }
            return std::nullopt;
        }
        refinement_t find_refinement_common(const std::vector<std::pair<state_t,label_t>>& X, const abstract_rule_t& abstract_rule) {
            std::vector<std::pair<state_t,label_t>> Y;
            const Interface* interface = nullptr;
            for (const auto& rule : _rule_mapping.get_concrete_values(abstract_rule)) {
                assert(_state_mapping.map_id(rule._from_id) == abstract_rule._from);
                auto from_state = _state_mapping.get_concrete_value(rule._from_id);
                interface = from_state.interface();
                assert(from_state.ops_done());
                Y.emplace_back(from_state.interface(), rule.entry(_state_mapping)._top_label);
            }
            assert(interface != nullptr);
            auto [found, abstract_interface_id] = _interface_abstraction.exists(interface);
            assert(found);
            return make_pair_refinement<state_t,label_t>(X, Y, abstract_interface_id, abstract_rule._pre);
        }

        std::pair<size_t,std::vector<label_t>> compute_pop_post(const label_t& pre_label, pdaaal::op_t op, const label_t& op_label, const State& to_state) const {
            size_t additional_pops = 0;
            std::vector<label_t> post;
            if (to_state.ops_done()) {
                switch (op) {
                    case pdaaal::POP:
                        // empty post
                        break;
                    case pdaaal::NOOP:
                        post = std::vector<label_t>{pre_label};
                        break;
                    case pdaaal::SWAP:
                        post = std::vector<label_t>{op_label};
                        break;
                    case pdaaal::PUSH:
                        post = std::vector<label_t>{pre_label, op_label};
                        break;
                }
            } else {
                const auto& ops = to_state._inf->table().entries()[to_state._eid]._rules[to_state._rid]._ops;
                assert(to_state._opid == 0);
                assert(ops.size() > 1);
                assert((ops[0]._op == RoutingTable::op_t::POP && op == pdaaal::POP)
                    || (ops[0]._op == RoutingTable::op_t::PUSH && op == pdaaal::PUSH && ops[0]._op_label == op_label)
                    || (ops[0]._op == RoutingTable::op_t::SWAP && op == pdaaal::SWAP && ops[0]._op_label == op_label)
                    || (ops[0]._op == RoutingTable::op_t::SWAP && op == pdaaal::NOOP && ops[0]._op_label == op_label && pre_label == op_label));
                post = std::vector<label_t>{pre_label};
                for (const auto& action : ops) {
                    switch (action._op) {
                        case RoutingTable::op_t::POP:
                            if (post.empty()) {
                                additional_pops++;
                            } else {
                                post.pop_back();
                            }
                            break;
                        case RoutingTable::op_t::SWAP:
                            if (post.empty()) {
                                additional_pops++;
                                post.push_back(action._op_label);
                            } else {
                                post.back() = action._op_label;
                            }
                            break;
                        case RoutingTable::op_t::PUSH:
                            post.push_back(action._op_label);
                            break;
                    }
                }
            }
            return {additional_pops, post};
        }

    private:
        const pdaaal::RefinementMapping<const Interface*>& _interface_abstraction;
        const StateMapping& _state_mapping;
        const pdaaal::AbstractionMapping<rule_t,abstract_rule_t>& _rule_mapping;
        std::vector<configuration_range_t> _temps;
    };

}


#endif //AALWINES_CEGARNETWORKPDAFACTORY_H
