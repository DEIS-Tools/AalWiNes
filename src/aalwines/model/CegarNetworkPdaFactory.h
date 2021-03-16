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
#include <aalwines/model/NetworkTranslation.h>
#include <aalwines/utils/pointer_back_inserter.h>
#include <aalwines/utils/ranges.h>
#include <aalwines/model/EdgeStatus.h>

#include <utility>

#include <json.hpp>
using json = nlohmann::json;

namespace aalwines {

    template <pdaaal::refinement_option_t refinement_option, typename W_FN, typename W, typename C, typename A> class CegarNetworkPdaReconstruction;

    // FIXME: For now simple unweighted version.
    template<typename W_FN = std::function<void(void)>, typename W = typename W_FN::result_type, typename C = std::less<W>, typename A = pdaaal::add<W>>
    class CegarNetworkPdaFactory : public pdaaal::CegarPdaFactory<Query::label_t, W, C, A> {
        template <pdaaal::refinement_option_t refinement_option, typename W_FN_, typename W_, typename C_, typename A_>
        friend class CegarNetworkPdaReconstruction;
    public:
        using label_t = Query::label_t;
    private:
        using Translation = NetworkTranslationW<W_FN>;
        using NFA = pdaaal::NFA<label_t>; // TODO: Make link NFA different type from header NFA. (low priority...)
        using nfa_state_t = typename NFA::state_t;
        using a_op_t = std::tuple<pdaaal::op_t,size_t>;
        using a_ops_t = std::vector<a_op_t>;
        using table_t = pdaaal::ptrie_set<std::tuple<size_t, size_t, a_op_t, a_ops_t>>; // a(label), a(to), a(first_op), a(remaining_ops)
        using abstract_state_t = std::tuple<size_t, const nfa_state_t*, a_ops_t>;
        using parent_t = pdaaal::CegarPdaFactory<label_t, W, C, A>;
        using abstract_rule_t = typename parent_t::abstract_rule_t;
        static constexpr uint32_t abstract_wildcard_label() noexcept { return std::numeric_limits<uint32_t>::max(); }
    public:
        json& json_output;

        template<typename label_abstraction_fn_t>
        CegarNetworkPdaFactory(json& json_output, const Network& network, const Query& query, std::unordered_set<label_t>&& all_labels,
                               label_abstraction_fn_t&& label_abstraction_fn,
                               std::function<size_t(const Interface*)>&& interface_abstraction_fn)
        : parent_t(all_labels, std::forward<label_abstraction_fn_t>(label_abstraction_fn)), json_output(json_output),
              //_all_labels(std::move(all_labels)),
              _translation(query, network, [](){}),
              _query(query), //_failures(query.number_of_failures()),
              _interface_abstraction(pdaaal::AbstractionMapping<const Interface*,size_t>(std::move(interface_abstraction_fn), network.all_interfaces().begin(), network.all_interfaces().end())),
              _abstract_tables(_interface_abstraction.size())
        {
            static_assert(std::is_convertible_v<label_abstraction_fn_t,
                    std::function<decltype(std::declval<label_abstraction_fn_t>()(std::declval<const label_t&>()))(const label_t&)>>);
            initialize();
            json_output["cegar_iterations"] = 0;
        }

        void refine(std::variant<std::pair<pdaaal::Refinement<const Interface*>, pdaaal::Refinement<label_t>>, abstract_rule_t>&& refinement) {
            if (refinement.index() == 0) { // Normal interface/label refinement
                refine(std::get<0>(std::move(refinement)));
            } else {
                assert(refinement.index() == 1); // Spurious abstract rule that needs to be removed.
                add_spurious_rule(std::get<1>(std::move(refinement)));
                // Here we don't need to remake stuff, since build_pda takes care of not adding the spurious rule.
            }
        }
        void refine(std::pair<pdaaal::Refinement<const Interface*>, pdaaal::Refinement<label_t>>&& refinement) {
            auto new_interfaces_begin = _interface_abstraction.size();
            _interface_abstraction.refine(refinement.first);
            auto new_interfaces_end = _interface_abstraction.size();
            assert(new_interfaces_begin + refinement.first.partitions().size() - (refinement.first.partitions().empty() ? 0 : 1) == new_interfaces_end);

            refine_states(refinement.first.abstract_id, new_interfaces_begin, new_interfaces_end);

            auto new_labels_end = this->number_of_labels();
            auto new_labels_count = refinement.second.partitions().size();
            if (new_labels_count > 0) {
                new_labels_count--;
            }
            use_label_refinement(refinement.second.abstract_id, new_labels_end - new_labels_count, new_labels_end);

            // TODO: Optimization: Find a way to refine and reuse tables...
            remake_tables();
        }
        void refine(pdaaal::HeaderRefinement<label_t>&& header_refinement) {
            /* --Not yet used...
            auto new_labels_end = this->number_of_labels();
            for (auto it = header_refinement.refinements().crbegin(); it < header_refinement.refinements().crend(); ++it) { // Header refinements were applied in order, so we go back in reverse.
                const auto& refinement = *it;
                assert(!refinement.partitions().empty());
                auto new_labels_count = refinement.partitions().size() - 1; // New labels were made for all partitions except one, which kept the old label.
                auto new_labels_begin = new_labels_end - new_labels_count;
                auto old_label = refinement.abstract_id;
                assert(old_label < new_labels_begin);
                use_label_refinement(old_label, new_labels_begin, new_labels_end);
                assert(new_labels_end > new_labels_count);
                new_labels_end -= new_labels_count;
            }*/
            // TODO: Optimization: Find a way to refine and reuse tables...
            remake_tables();
        }

    protected:
        void build_pda() override {
            // Since we don't reset states when refining (to keep indexes consistent) we don't a priori know which states are still valid.
            // For states with ops.empty() we use _edges to check validity, for !ops.empty() we store the reachable states here.
            // Keeping all states can increase the PDA size a bit, since the PDA datastructure assumes consecutive state ids, i.e. it will also create entries for the unused states.
            std::unordered_set<size_t> other_states;

            size_t count_rules = 0;
            for (size_t from_state = 0; from_state < _abstract_states.size(); ++from_state) {
                auto [a_inf, nfa_state, ops] = _abstract_states.at(from_state);
                if (ops.empty()) {
                    assert(from_state < _num_states_no_ops);
                    const auto& table = _abstract_tables[a_inf]; // TODO: Add begin(),end() to pdaaal::ptrie_set...
                    for (size_t table_i = 0; table_i < table.size(); ++table_i) {
                        auto [label, a_to_inf, first_op, other_ops] = table.at(table_i);
                        auto it = _edges.find(std::make_tuple(a_inf, nfa_state, a_to_inf));
                        if (it == _edges.end()) continue;
                        for (const auto& to_nfa_state : it->second) {
                            auto to_state = _abstract_states.insert({a_to_inf, to_nfa_state, other_ops}).second;
                            if (!other_ops.empty()) { // Remember that to_state is reachable and handle later.
                                other_states.emplace(to_state);
                            }
                            abstract_rule_t rule(from_state, label, to_state, std::get<0>(first_op), std::get<1>(first_op));
                            if (is_spurious_rule(rule)) continue;
                            if (label == abstract_wildcard_label()) {
                                this->add_wildcard_rule(rule);
                            } else {
                                this->add_rule(rule);
                            }
                            count_rules++;
                        }
                    }
                }
            }
            for (size_t from_state : other_states) {
                auto [a_inf, nfa_state, ops] = _abstract_states.at(from_state);
                assert(!ops.empty());
                auto first_op = ops[0];
                auto to_state = _abstract_states.insert({a_inf, nfa_state, a_ops_t(ops.begin()+1, ops.end())}).second;
                abstract_rule_t rule(from_state, abstract_wildcard_label(), to_state, std::get<0>(first_op), std::get<1>(first_op));
                this->add_wildcard_rule(rule);
                count_rules++;
            }

            json_output["rules"] = count_rules;
            json_output["labels"] = this->number_of_labels();
            json_output["interfaces"] = _interface_abstraction.size();
            json_output["cegar_iterations"] = json_output["cegar_iterations"].get<int>() + 1;
            //std::cout << "; rules: " << count_rules << "; labels: " << this->number_of_labels() << "; (interfaces: " << _interface_abstraction.size() << ")" << std::endl; // FIXME: Remove...
        }
        const std::vector<size_t>& initial() override {
            return _initial;
        }
        const std::vector<size_t>& accepting() override {
            return _accepting;
        }

    private:
        template<bool initial=false>
        void add_state(const nfa_state_t* nfa_state, const Interface* inf, size_t a_inf) {
            auto [abstract_fresh, abstract_id] = _abstract_states.insert({a_inf, nfa_state, a_ops_t{}});
            if (abstract_fresh) {
                if (initial) {
                    _initial.push_back(abstract_id);
                }
                if (nfa_state->_accepting && !inf->is_virtual()) { // We assume that the interface abstraction always distinguishes virtual and non-virtual interfaces.
                    _accepting.push_back(abstract_id);
                }
            }
        }

        void initialize() {
            std::unordered_set<std::pair<const Interface*, const nfa_state_t*>,
                    boost::hash<std::pair<const Interface*, const nfa_state_t*>>> seen;
            std::vector<std::tuple<const Interface*, const nfa_state_t*,size_t>> waiting;

            _translation.make_initial_states([&seen,&waiting,this](const Interface* inf, const std::vector<nfa_state_t*>& next) {
                auto a_inf = _interface_abstraction.exists(inf).second;
                for (const auto& n : next) {
                    if (seen.emplace(inf, n).second) {
                        waiting.emplace_back(inf, n, a_inf);
                        add_state<true>(n, inf, a_inf);
                    }
                }
            });
            make_edges<true>(std::move(seen), std::move(waiting));
        }

        void refine_states(size_t old_interface, size_t new_interfaces_begin, size_t new_interfaces_end) {
            if (new_interfaces_begin == new_interfaces_end) return; // No interface refinement.

            // This refinement of states is similar to the result of resetting all and rerunning initialize(),
            // except that the ids of abstract states are kept stable.
            // This is useful for keeping _spurious_rules consistent.

            std::unordered_set<std::pair<const Interface*, const nfa_state_t*>,
                    boost::hash<std::pair<const Interface*, const nfa_state_t*>>> seen;
            std::vector<std::tuple<const Interface*, const nfa_state_t*,size_t>> waiting;

            // The refinement might make some old _initial states stop being initial.
            // Here we find nfa_states s such that (old_interface, s) was initial.
            std::vector<const nfa_state_t*> old_initial;
            std::vector<const nfa_state_t*> new_initial;
            std::vector<const nfa_state_t*> temp;

            _translation.make_initial_states([&,this](const Interface* inf, const std::vector<nfa_state_t*>& next) {
                assert(std::is_sorted(next.begin(), next.end()));
                auto a_inf = _interface_abstraction.exists(inf).second;
                bool is_new = new_interfaces_begin <= a_inf && a_inf < new_interfaces_end;
                for (const auto& n : next) {
                    if (seen.emplace(inf, n).second) {
                        waiting.emplace_back(inf, n, a_inf);
                        if (is_new) {
                            add_state<true>(n, inf, a_inf); // We only add new states.
                        }
                    }
                }
                if (a_inf == old_interface) {
                    temp.clear();
                    std::set_union(old_initial.begin(), old_initial.end(), next.begin(), next.end(), std::back_inserter(temp));
                    std::swap(old_initial, temp);
                } else if (is_new) {
                    temp.clear();
                    std::set_union(new_initial.begin(), new_initial.end(), next.begin(), next.end(), std::back_inserter(temp));
                    std::swap(new_initial, temp);
                }
            });

            // Use old_initial and new_initial to figure out if some states from _initial should no longer be initial. Remove them.
            std::vector<const nfa_state_t*> diff;
            std::set_difference(new_initial.begin(), new_initial.end(), old_initial.begin(), old_initial.end(), std::back_inserter(diff));
            for (const auto& nfa_state : diff) {
                assert(_abstract_states.exists({old_interface, nfa_state, a_ops_t{}}).first);
                auto state = _abstract_states.exists({old_interface, nfa_state, a_ops_t{}}).second;
                auto it = std::find(_initial.begin(), _initial.end(), state);
                assert(it != _initial.end());
                _initial.erase(it);
            }

            // Nothing fancy here, just clear and remake the edges.
            _edges.clear();
            make_edges(std::move(seen), std::move(waiting), [new_interfaces_begin, new_interfaces_end](size_t a_inf){
                return new_interfaces_begin <= a_inf && a_inf < new_interfaces_end;
            });

            // TODO: (Maybe) Refine spurious rules that match  old_interface -> [new_interfaces_begin, new_interfaces_end).
        }
        void use_label_refinement(size_t old_label, size_t new_label_begin, size_t new_label_end) {
            // TODO: Use old_label -> [new_labels_begin, new_labels_end)
            // TODO: (Maybe) Refine spurious rules that match this refinement.
        }

        void add_spurious_rule(abstract_rule_t&& rule) {
            _spurious_rules.insert(rule);
        }
        bool is_spurious_rule(const abstract_rule_t& rule) {
            return _spurious_rules.exists(rule).first;
        }

        template<bool first_time = false>
        void process_table(const Interface* inf, size_t a_inf) {
            auto label_abstraction = [this](const label_t& label) { return this->abstract_label(label); };
            assert(a_inf < _abstract_tables.size());
            std::unordered_set<const Interface*>* out_set;
            if constexpr (first_time) {
                out_set = &_out_infs.try_emplace(inf).first->second;
            }
            for (const auto& entry : inf->table()->entries()) {
                auto label = abstract_pre_label(entry._top_label);
                for (const auto& forward : entry._rules) {
                    if (forward._priority > _query.number_of_failures()) continue; // TODO: Approximation here.
                    if constexpr (first_time) {
                        out_set->emplace(forward._via);
                    }
                    assert(_interface_abstraction.exists(forward._via->match()).first);
                    auto to = _interface_abstraction.exists(forward._via->match()).second;
                    auto first_op = forward.first_action(label_abstraction);
                    a_ops_t ops;
                    for (size_t i = 1; i < forward._ops.size(); ++i) {
                        ops.emplace_back(forward._ops[i].convert_to_pda_op(label_abstraction));
                    }
                    _abstract_tables[a_inf].insert({label,to,first_op,std::move(ops)});
                }
            }
        }

        template<bool first_time = false>
        void make_edges(std::unordered_set<std::pair<const Interface*, const nfa_state_t*>, boost::hash<std::pair<const Interface*, const nfa_state_t*>>>&& seen,
                        std::vector<std::tuple<const Interface*, const nfa_state_t*,size_t>>&& waiting,
                        const std::function<bool(size_t)>& is_new = [](const auto&){return true;}) {
            auto add = [&seen, &waiting, &is_new, this](const nfa_state_t* n, const Interface* inf, size_t a_inf) {
                if (seen.emplace(inf, n).second) {
                    waiting.emplace_back(inf, n, a_inf);
                    if constexpr (first_time) {
                        add_state(n, inf, a_inf);
                    } else {
                        if (is_new(a_inf)) {
                            add_state(n, inf, a_inf);
                        }
                    }
                }
            };
            _relevant_tables.clear();
            while (!waiting.empty()) {
                auto [inf, nfa_state, a_inf] = waiting.back();
                waiting.pop_back();

                if (_relevant_tables.emplace(inf).second) {
                    if constexpr (first_time) { // First time we need to process tables to construct _out_infs, but later we defer process_table to the remake_tables() function.
                        process_table<first_time>(inf, a_inf);
                    }
                }
                assert(_out_infs.find(inf) != _out_infs.end());
                for (const Interface* out_inf : _out_infs.find(inf)->second) {
                    auto to_inf = out_inf->match();
                    auto a_to_inf = _interface_abstraction.exists(to_inf).second;
                    if (out_inf->is_virtual()) {
                        _edges.emplace(std::make_tuple(a_inf,nfa_state,a_to_inf), nfa_state);
                        add(nfa_state, to_inf, a_to_inf);
                    } else {
                        for (const auto& e : nfa_state->_edges) {
                            for (const auto& n : e.follow_epsilon()) {
                                if (!e.contains(out_inf->global_id())) continue;
                                _edges.emplace(std::make_tuple(a_inf,nfa_state,a_to_inf), n);
                                add(n, to_inf, a_to_inf);
                            }
                        }
                    }
                }
            }
        }

        void remake_tables() { // Used after the first time.
            _abstract_tables.clear();
            _abstract_tables = std::vector<table_t>(_interface_abstraction.size());
            for (const auto& inf : _relevant_tables) {
                process_table(inf, _interface_abstraction.exists(inf).second);
            }
        }

        uint32_t abstract_pre_label(const label_t& pre_label) const {
            assert(pre_label == Query::wildcard_label() || this->abstract_label(pre_label).first);
            return (pre_label == Query::wildcard_label())
                    ? abstract_wildcard_label()
                    : this->abstract_label(pre_label).second;
        }

    private:
        //std::unordered_set<label_t> _all_labels;
        Translation _translation; // TODO: Figure out how to use common parts from Translation.
        const Query& _query;
        pdaaal::RefinementMapping<const Interface*> _interface_abstraction; // This is what gets refined by CEGAR.

        std::vector<table_t> _abstract_tables;
        pdaaal::ptrie_set<abstract_state_t> _abstract_states;

        // Keeps track of which tables (interfaces) have been processed, and remembers for next time (when useful)...
        std::unordered_set<const Interface* /* TODO: const RoutingTable* ... */> _relevant_tables;

        // The idea is to only go through concrete tables once and only go through (concrete) path regex once.
        // Combine (product) abstracted versions of tables and path.
        std::unordered_map<const Interface*, /* TODO: const RoutingTable* */  // out_infs[e].contains(e')  iff  (e',..) \in \tau(e,..).
                std::unordered_set<const Interface*>> _out_infs; // Constructed first time, never changed after that.

        pdaaal::fut::set<std::tuple<std::tuple<size_t, const nfa_state_t*, size_t>, const nfa_state_t*>,
                pdaaal::fut::type::hash, pdaaal::fut::type::vector> _edges; // Given (a(e),s,a(e')) Lists all s' such that (e,s) -> (e',s'), i.e. s --e'-> s' and e' \in out_infs[e]

        pdaaal::ptrie_set<abstract_rule_t> _spurious_rules;
        std::vector<size_t> _initial;
        std::vector<size_t> _accepting;
    };

    using cegar_configuration_t = std::tuple<pdaaal::Header<Query::label_t>,              // Current header (possibly set of headers represented using wildcards)
                                             const Interface*,                            // Current link
                                             const pdaaal::NFA<Query::label_t>::state_t*, // State in the path NFA
                                             const RoutingTable::entry_t*,                // Entry and ..
                                             const RoutingTable::forward_t*,              // .. forwarding rule is used when reconstructing trace
                                             std::vector<Query::label_t>,                // In some cases, if wildcard labels where specialized, we need to know which in order to reconstruct trace.
                                             EdgeStatus>;

    using ConfigurationRange = utils::VariantRange< // TODO: We could make this simpler / more direct, but it works for now...
            utils::VectorRange<utils::SingletonRange<const Interface*>, cegar_configuration_t>,
            utils::VectorRange<utils::FilterRange<typename pdaaal::RefinementMapping<const Interface*>::concrete_value_range>, cegar_configuration_t>,
            utils::SingletonRange<cegar_configuration_t>>;

    struct json_wrapper {
        // json's implicit type conversions messes up with std::variant in Clang10.
        // So we put the json in a wrapping.
        explicit json_wrapper(json&& j) : _json(std::move(j)) {};
        json&& get() && { return std::move(_json); }
        json _json;
    };

    // For now simple unweighted version.
    template<pdaaal::refinement_option_t refinement_option, typename W_FN = std::function<void(void)>, typename W = typename W_FN::result_type, typename C = std::less<W>, typename A = pdaaal::add<W>>
    class CegarNetworkPdaReconstruction : public pdaaal::CegarPdaReconstruction<
            Query::label_t, // label_t
            const Interface*, // state_t
            ConfigurationRange,
            json_wrapper , // concrete_trace_t
            W, C, A> {
        friend class CegarNetworkPdaFactory<W_FN,W,C,A>;
        using factory_t = CegarNetworkPdaFactory<W_FN,W,C,A>;
    public:
        using label_t = typename factory_t::label_t;
        using concrete_trace_t = json_wrapper;
    private:
        using Translation = typename factory_t::Translation;
        using state_t = const Interface*; // This is the 'state' that is refined.
        using abstract_state_t = typename factory_t::abstract_state_t;
        using header_t = pdaaal::Header<Query::label_t>;
        using configuration_range_t = ConfigurationRange;
        using configuration_t = typename configuration_range_t::value_type; // = cegar_configuration_t;
        using parent_t = pdaaal::CegarPdaReconstruction<label_t, state_t, configuration_range_t, concrete_trace_t, W, C, A>;
        using abstract_rule_t = typename parent_t::abstract_rule_t;
        using NFA = pdaaal::NFA<Query::label_t>;
        using nfa_state_t = NFA::state_t;
        using solver_instance_t = typename parent_t::solver_instance_t;
    public:
        using refinement_t = typename parent_t::refinement_t;
        using header_refinement_t = typename parent_t::header_refinement_t;

        explicit CegarNetworkPdaReconstruction(const factory_t& factory, const solver_instance_t& instance, const pdaaal::NFA<label_t>& initial_headers, const pdaaal::NFA<label_t>& final_headers)
        : parent_t(instance, initial_headers, final_headers), _factory(factory) {};

    protected:

        configuration_range_t initial_concrete_rules(const abstract_rule_t& abstract_rule) override {
            auto [a_inf, nfa_state, ops] = _factory._abstract_states.at(abstract_rule._from);
            assert(ops.empty()); // Initial state must have empty ops.
            auto to_state = _factory._abstract_states.at(abstract_rule._to);
            size_t ops_size = std::get<2>(to_state).size() + (abstract_rule._op == pdaaal::op_t::NOOP ? 0 : 1);
            assert(abstract_rule._op != pdaaal::op_t::NOOP || ops_size == 0);

            return utils::VectorRange<utils::FilterRange<typename pdaaal::RefinementMapping<const Interface*>::concrete_value_range>, configuration_t>(
                utils::FilterRange(
                    _factory._interface_abstraction.get_concrete_values_range(a_inf), // Inner range of interfaces
                    [this,nfa_state=nfa_state](const auto& inf){ return !inf->match()->is_virtual() && NFA::has_as_successor(_factory._query.path().initial(), inf->match()->global_id(), nfa_state); }), // Filter predicate
                [this, header=this->initial_header(), &abstract_rule, nfa_state=nfa_state, &to_state, ops_size](const auto& inf){
                    return make_configurations(header, abstract_rule, inf, nfa_state, to_state, ops_size, EdgeStatus()); // Transform interface to vector of configurations.
            });
        }
        refinement_t find_initial_refinement(const abstract_rule_t& abstract_rule) override {
            std::vector<std::pair<state_t,label_t>> X;
            auto [a_inf, nfa_state, ops] = _factory._abstract_states.at(abstract_rule._from);
            auto labels = this->pre_labels(this->initial_header());
            for (const auto& inf : _factory._interface_abstraction.get_concrete_values_range(a_inf)) {
                if (inf->match()->is_virtual() || !NFA::has_as_successor(_factory._query.path().initial(), inf->match()->global_id(), nfa_state)) continue; // concrete state is initial
                for (const auto& label : labels) {
                    if (this->label_maps_to(label, abstract_rule._pre)) {
                        X.emplace_back(inf, label);
                    }
                }
            }
            return find_refinement_common(std::move(X), abstract_rule, a_inf, nfa_state);
        }
        configuration_range_t search_concrete_rules(const abstract_rule_t& abstract_rule, const configuration_t& conf) override {
            auto inf = get_interface(conf);
            assert(_factory._interface_abstraction.maps_to(inf, std::get<0>(_factory._abstract_states.at(abstract_rule._from))));
            if (std::get<2>(_factory._abstract_states.at(abstract_rule._from)).empty()) {
                auto to_state = _factory._abstract_states.at(abstract_rule._to);
                size_t ops_size = std::get<2>(to_state).size() + (abstract_rule._op == pdaaal::op_t::NOOP ? 0 : 1);
                assert(abstract_rule._op != pdaaal::op_t::NOOP || ops_size == 0);

                return utils::VectorRange<utils::SingletonRange<const Interface*>, configuration_t>(
                    utils::SingletonRange<const Interface*>(inf),
                    [this, &conf, &abstract_rule, nfa_state=std::get<2>(conf), &to_state, ops_size](const auto& inf){
                        return make_configurations(std::get<0>(conf), abstract_rule, inf, nfa_state, to_state, ops_size, std::get<6>(conf)); // Transform interface to vector of configurations.
                    });
            } else {
                return utils::SingletonRange<configuration_t>(std::get<0>(conf), inf, std::get<2>(conf), nullptr, nullptr, std::vector<label_t>(), std::get<6>(conf));
            }
        }
        refinement_t find_refinement(const abstract_rule_t& abstract_rule, const std::vector<configuration_t>& configurations) override {
            std::vector<std::pair<state_t,label_t>> X;
            assert(std::get<2>(_factory._abstract_states.at(abstract_rule._from)).empty());
            auto [a_inf, nfa_state, ops] = _factory._abstract_states.at(abstract_rule._from);
            assert(ops.empty());
            std::unordered_set<const Interface*> interfaces_with_wildcard_headers;
            std::optional<std::vector<label_t>> labels_matching_wildcard;
            for (const auto& conf : configurations) { // [header, old_inf, c_nfa_state, e, r, l]
                auto inf = get_interface(conf);
                const auto& header = std::get<0>(conf);
                if (nfa_state != std::get<2>(conf) || !_factory._interface_abstraction.maps_to(inf, a_inf)) continue;
                if (!header.empty() && !header.top_is_concrete()) { // Check if header has wildcard on top.
                    if (interfaces_with_wildcard_headers.emplace(inf).second) { // Only add to X if interface is fresh here. (Not enough to ensure X has no duplicates, but it helps...)
                        if (!labels_matching_wildcard) { // Only compute this once, since it is the same for all headers with wildcard on top.
                            labels_matching_wildcard = this->pre_labels(header);
                            labels_matching_wildcard->erase(std::remove_if(labels_matching_wildcard->begin(), labels_matching_wildcard->end(),
                                [this,&abstract_rule](const auto& label){ return !this->label_maps_to(label, abstract_rule._pre); }),
                                labels_matching_wildcard->end());
                        }
                        for (const auto& label : labels_matching_wildcard.value()) {
                            X.emplace_back(inf, label);
                        }
                    }
                } else {
                    for (const auto& label : this->pre_labels(header)) {
                        if (this->label_maps_to(label, abstract_rule._pre)) {
                            X.emplace_back(inf, label);
                        }
                    }
                }
            }
            // Sort and remove duplicates from X.
            std::sort(X.begin(), X.end());
            X.erase(std::unique(X.begin(), X.end()), X.end());
            return find_refinement_common(std::move(X), abstract_rule, a_inf, nfa_state);
        }
        header_t get_header(const configuration_t& conf) override {
            return std::get<0>(conf);
        }

        concrete_trace_t get_concrete_trace(std::vector<configuration_t>&& configurations, std::vector<label_t>&& final_header, size_t initial_abstract_state) override {
            json trace = json::array();
            size_t remaining_pops = 0;
            for (auto it = configurations.crbegin(); it < configurations.crend(); ++it) {
                const auto& [header, from_inf, nfa_state, entry, forward, labels, edge_status] = *it;
                if (forward == nullptr) continue;
                assert(entry != nullptr);
                if (remaining_pops > 0) {
                    // We need this header to finalize the last step.
                    // We know that the top 'remaining_pops' labels of 'header' is concrete, since otherwise it would have been recorded in 'labels' in last step.
                    assert(header.concrete_part.size() >= remaining_pops);
                    final_header.insert(final_header.end(), header.concrete_part.end() - remaining_pops, header.concrete_part.end());
                    remaining_pops = 0;
                }
                auto to_inf = forward->_via->match();
                Translation::add_link_to_trace(trace, to_inf, final_header);
                _factory._translation.add_rule_to_trace(trace, from_inf, *entry, *forward);
                const auto& ops = forward->_ops;
                auto [post, pops] = compute_pop_post(entry->_top_label, ops);
#ifndef NDEBUG
                assert(final_header.size() >= post.size());
                for (size_t i = 0; i < post.size(); ++i) { // Assert that top of final_header matches post.
                    assert(post[i] == final_header[i + final_header.size() - post.size()]);
                }
#endif
                final_header.erase(final_header.end() - post.size(), final_header.end()); // Remove what was pushed (i.e. post)
                for (auto label_it = labels.crbegin(); label_it < labels.crend(); ++label_it) { // If wildcards was specialized and popped, push the corresponding valid labels.
                    final_header.push_back(*label_it);
                    assert(pops > 0);
                    pops--; // Keep count of how many pops we have left to reverse.
                }
                if (pops == 0 && !entry->ignores_label()) { // Standard case where top label is normal, and we didn't pop more.
                    final_header.push_back(entry->_top_label);
                }
                if (pops > 0) { // We popped more than we have pushed on now.
                    remaining_pops = pops; // Remember this, and handle it using the previous header (next iteration due to reverse iterator).
                }
            }
            assert(remaining_pops == 0);
            const Interface* first_inf = nullptr;
            if (configurations.empty()) {
                auto [a_inf, nfa_state, ops] = _factory._abstract_states.at(initial_abstract_state);
                for (const auto& inf : _factory._interface_abstraction.get_concrete_values_range(a_inf)) {
                    if (NFA::has_as_successor(_factory._query.path().initial(), inf->match()->global_id(), nfa_state)) {
                        first_inf = inf;
                        break;
                    }
                }
                assert(first_inf != nullptr);
            } else {
                first_inf = std::get<1>(configurations[0]);
            }
            Translation::add_link_to_trace(trace, first_inf, final_header);
            std::reverse(trace.begin(), trace.end());
            return json_wrapper(std::move(trace));
        }
    private:
        static const Interface* get_interface(const configuration_t& conf) {
            return (std::get<4>(conf) == nullptr) ? std::get<1>(conf) : std::get<4>(conf)->_via->match();
        }
        std::vector<const RoutingTable::entry_t*> get_entries_matching(const header_t& header, const Interface* inf) const {
            auto labels = this->pre_labels(header);
            assert(std::is_sorted(labels.begin(), labels.end()));
            return get_entries_matching(labels, inf);
        }
        std::vector<const RoutingTable::entry_t*> get_entries_matching(const std::vector<label_t>& labels, const Interface* inf) const {
            std::vector<const RoutingTable::entry_t*> matching_entries;
            std::set_intersection(inf->table()->entries().begin(), inf->table()->entries().end(),
                                  labels.begin(), labels.end(), pointer_back_inserter(matching_entries), RoutingTable::CompEntryLabel());
            if (!inf->table()->entries().empty() && inf->table()->entries().back().ignores_label() && (matching_entries.empty() || !matching_entries.back()->ignores_label())) {
                matching_entries.emplace_back(&inf->table()->entries().back());
            }
            return matching_entries;
        }

        bool matches_first_action(const RoutingTable::forward_t& forward, const abstract_rule_t& abstract_rule) {
            auto [op, op_label] = forward.first_action();
            if (op != abstract_rule._op) return false;
            switch (op) {
                case pdaaal::op_t::NOOP:
                    assert(forward._ops.empty());
                case pdaaal::op_t::POP:
                    break;
                case pdaaal::op_t::SWAP:
                case pdaaal::op_t::PUSH:
                    if (!this->label_maps_to(op_label, abstract_rule._op_label)) return false;
                    break;
            }
            return true;
        }
        bool matches_action(std::pair<pdaaal::op_t,label_t>&& action_op, const typename factory_t::a_op_t& op) {
            return (std::get<0>(action_op) == std::get<0>(op) && (std::get<0>(action_op) == pdaaal::op_t::POP || this->label_maps_to(std::get<1>(action_op), std::get<1>(op))));
        }
        bool matches_other_actions(const RoutingTable::forward_t& forward, const typename factory_t::a_ops_t ops) {
            for (size_t i = 1; i < forward._ops.size(); ++i) {
                if (!matches_action(forward._ops[i].convert_to_pda_op(), ops[i-1])) return false;
            }
            return true;
        }
        bool matches_forward(const RoutingTable::forward_t& forward, const abstract_rule_t& abstract_rule,
                             const nfa_state_t* from_nfa_state, const abstract_state_t& to_state, size_t ops_size) {
            const auto& [to_a_inf, to_nfa_state, to_ops] = to_state;
            return ops_size == forward._ops.size() &&
                   forward._priority <= _factory._query.number_of_failures() && // TODO: Approximation here.
                   matches_first_action(forward, abstract_rule) &&
                   _factory._interface_abstraction.maps_to(forward._via->match(), to_a_inf) &&
                   NFA::has_as_successor(from_nfa_state, forward._via->global_id(), to_nfa_state) &&
                   matches_other_actions(forward, to_ops);
        }

        std::vector<configuration_t> make_configurations(const header_t& header, const abstract_rule_t& abstract_rule,
                                                         const Interface* inf, const nfa_state_t* nfa_state,
                                                         const abstract_state_t& to_state, size_t ops_size, const EdgeStatus& edge_status) {
            assert(edge_status.soundness_check(_factory._query.number_of_failures()));
            std::vector<configuration_t> result;
            for (const RoutingTable::entry_t* entry : get_entries_matching(header, inf)) {
                for (const auto& forward : entry->_rules) {
                    if (!matches_forward(forward, abstract_rule, nfa_state, to_state, ops_size)) continue;
                    auto next_edge_status = edge_status.next_edge_state(*entry, forward, _factory._query.number_of_failures());
                    if (!next_edge_status.has_value()) continue;
                    auto [post, additional_pops] = compute_pop_post(entry->_top_label, forward._ops);
                    std::optional<std::pair<header_t,std::vector<label_t>>> new_header;
                    if (entry->ignores_label()) { // Wildcard pre_label
                        new_header = this->update_header_wildcard_pre(header, post, additional_pops);
                    } else {
                        new_header = this->update_header(header, entry->_top_label, post, additional_pops);
                    }
                    if (!new_header.has_value()) continue;
                    result.emplace_back(new_header.value().first, inf, std::get<1>(to_state), entry, &forward, new_header.value().second, std::move(next_edge_status).value());
                }
            }
            return result;
        }

        refinement_t find_refinement_common(std::vector<std::pair<state_t,label_t>>&& X, const abstract_rule_t& abstract_rule, size_t a_inf, const nfa_state_t* nfa_state) {
            std::vector<std::pair<state_t,label_t>> Y;
            std::vector<state_t> Y_wildcard;
            auto to_state = _factory._abstract_states.at(abstract_rule._to);
            size_t ops_size = std::get<2>(to_state).size() + (abstract_rule._op == pdaaal::op_t::NOOP ? 0 : 1);
            assert(abstract_rule._op != pdaaal::op_t::NOOP || ops_size == 0);

            auto labels = this->get_concrete_labels(abstract_rule._pre);
            std::sort(labels.begin(), labels.end());
            for (const auto& inf : _factory._interface_abstraction.get_concrete_values(a_inf)) {
                if (_factory._out_infs.find(inf) == _factory._out_infs.end()) continue;
                for (const RoutingTable::entry_t* entry : get_entries_matching(labels, inf)) {
                    if (std::any_of(entry->_rules.begin(), entry->_rules.end(), // Check if any forwarding rule on this entry matches abstract rule.
                                    [&](const auto& forward){ return matches_forward(forward, abstract_rule, nfa_state, to_state, ops_size); })) {
                        if (entry->ignores_label()) {
                            assert(Y_wildcard.empty() || Y_wildcard.back() != inf); // At most one entry->ignores_label() per interface.
                            Y_wildcard.emplace_back(inf);
                            break; // inf \in Y_wildcard covers all (inf,label) pairs that could be added to Y later, so break now.
                        } else {
                            assert(this->label_maps_to(entry->_top_label, abstract_rule._pre));
                            Y.emplace_back(inf, entry->_top_label);
                        }
                    }
                }
            }
            if (Y.empty() && Y_wildcard.empty()) {
                return abstract_rule; // This is a spurious rule (i.e. it has no matching concrete rules), so remove it (and remember it's removal for later).
            } // else
            if (!Y_wildcard.empty()) {
                std::sort(Y_wildcard.begin(), Y_wildcard.end());
                Y.erase(std::remove_if(Y.begin(), Y.end(), [&Y_wildcard](const auto& p){
                    auto lb = std::lower_bound(Y_wildcard.begin(), Y_wildcard.end(), p.first);
                    return lb != Y_wildcard.end() && *lb == p.first; // Remove from Y elements with interface in Y_wildcard, since they are covered by the wildcard.
                }), Y.end());
            }
            return pdaaal::make_refinement<refinement_option>(std::move(X), std::move(Y), a_inf, abstract_rule._pre, std::move(Y_wildcard));
        }

        std::pair<std::vector<label_t>, size_t> compute_pop_post(const label_t& pre_label, const std::vector<RoutingTable::action_t>& ops) const {
            std::vector<label_t> post;
            size_t additional_pops = 0;
            if (pre_label != Query::wildcard_label()) {
                post.emplace_back(pre_label);
            }
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
            return {post, additional_pops};
        }

    private:
        const factory_t& _factory;
    };

}

#endif //AALWINES_CEGARNETWORKPDAFACTORY_H
