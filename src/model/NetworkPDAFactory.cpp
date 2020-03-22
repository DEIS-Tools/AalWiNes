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
 * File:   NetworkPDAFactory.cpp
 * Author: Peter G. Jensen <root@petergjoel.dk>
 * 
 * Created on July 19, 2019, 6:26 PM
 */

#include "NetworkPDAFactory.h"
#include "pdaaal/model/PDAFactory.h"

namespace aalwines
{

    NetworkPDAFactory::NetworkPDAFactory(Query& query, Network& network, bool only_mpls_swap)
    : PDAFactory(query.construction(), query.destruction(), network.all_labels()), _network(network), _query(query), _path(query.path()), _only_mpls_swap(only_mpls_swap)
    {
        // add special NULL state initially
        NFA::state_t* ns = nullptr;
        Interface* nr = nullptr;
        add_state(ns, nr);
        _path.compile();
        construct_initial();
    }

    void NetworkPDAFactory::construct_initial()
    {
        // there is potential for some serious pruning here!
        auto add_initial = [&, this](NFA::state_t* state, const Interface * inf)
        {
            if(inf != nullptr && inf->is_virtual()) // don't start on a virtual interface.
                return;
            std::vector<NFA::state_t*> next{state};
            NFA::follow_epsilon(next);
            for (auto& n : next) {
                auto res = add_state(n, inf, 0, 0, 0, -1, true);
                if (res.first)
                    _initial.push_back(res.second);
            }
        };

        for (auto i : _path.initial()) {
            // the path is one behind, we are coming from an unknown router via an OK interface
            // i.e. we have to move straight to the next state
            for (auto& e : i->_edges) {
                if (e.wildcard(_network.all_interfaces().size())) {
                    for (auto inf : _network.all_interfaces()) {
                        
                        add_initial(e._destination, inf->match());
                    }
                }
                else if (!e._negated) {
                    for (auto s : e._symbols) {
                        assert(s.type() == Query::INTERFACE);
                        auto inf = _network.all_interfaces()[s.value()];
                        add_initial(e._destination, inf->match());
                    }
                }
                else {
                    for (auto inf : _network.all_interfaces()) {
                        auto iid = Query::label_t{Query::INTERFACE, 0, inf->global_id()};
                        auto lb = std::lower_bound(e._symbols.begin(), e._symbols.end(), iid);
                        assert(std::is_sorted(e._symbols.begin(), e._symbols.end()));
                        if (lb == std::end(e._symbols) || *lb != iid)
                        {
                            add_initial(e._destination, inf->match());
                        }
                    }
                }
            }
        }
        std::sort(_initial.begin(), _initial.end());
    }

    std::pair<bool, size_t> NetworkPDAFactory::add_state(NFA::state_t* state, const Interface* inf, int32_t mode, int32_t eid, int32_t fid, int32_t op, bool initial)
    {
        nstate_t ns;
        ns._appmode = mode;
        ns._nfastate = state;
        ns._opid = op;
        ns._inf = inf;
        ns._rid = fid;
        ns._eid = eid;
        ns._initial = initial && _only_mpls_swap; // only need for this when restricting labels
        auto res = _states.insert(ns);
        if (res.first) {
            if(res.second != 0)
            {
                // don't check null-state
                auto& d = _states.get_data(res.second);
                d = op == -1 && (inf == nullptr || !inf->is_virtual()) ? state->_accepting : false;
            }
        }
        /*if(res.first) std::cerr << "## NEW " << std::endl;
            std::string rn;
            if(inf)
                rn = inf->source()->name();
            else
                rn = "SINK";
            std::cerr << "ADDED STATE " << state << " R " << rn << "(" << inf << ")" << " M" << mode << " F" << fid << " O" << op << " E" << eid << std::endl;
            std::cerr << "\tID " << res.second << std::endl;
            if(_states.get_data(res.second))
            std::cerr << "\t\tACCEPTING !" << std::endl;*/

        return res;
    }

    const std::vector<size_t>& NetworkPDAFactory::initial()
    {
        return _initial;
    }

    bool NetworkPDAFactory::empty_accept() const
    {
        for (auto s : _path.initial()) {
            if (s->_accepting)
                return true;
        }
        return false;
    }

    bool NetworkPDAFactory::accepting(size_t i)
    {
        return _states.get_data(i);
    }

    int32_t NetworkPDAFactory::set_approximation(const nstate_t& state, const RoutingTable::forward_t& forward)
    {
        auto num_fail = _query.number_of_failures();
        auto err = std::numeric_limits<int32_t>::max();
        switch (_query.approximation()) {
        case Query::OVER:
            if ((int)forward._weight > num_fail)
                return err;
            else
                return 0;
        case Query::UNDER:
        {
            auto nm = state._appmode + forward._weight;
            if ((int)nm > num_fail)
                return err;
            else
                return nm;
        }
        case Query::EXACT:
        case Query::DUAL:
        default:
            return err;
        }
    }

    void NetworkPDAFactory::expand_back(std::vector<rule_t>& rules, const Query::label_t& pre)
    {
        rule_t cpy;
        std::swap(rules.back(), cpy);
        rules.pop_back();
        auto val = pre.value();
        auto mask = pre.mask();
        
        switch(pre.type())
        {
        case Query::ANYSTICKY:
        case Query::ANYMPLS:
            mask = 64;
            val = 0;
            // fall through to MPLS
        case Query::STICKY_MPLS:
        case Query::MPLS:
        {
            if(mask == 0)
            {
                rules.push_back(cpy);
                rules.back()._pre = pre;
                rules.push_back(cpy);
                if(pre.type() & Query::STICKY)
                    rules.back()._pre = Query::label_t::any_sticky_mpls;
                else
                    rules.back()._pre = Query::label_t::any_mpls;
            }
            else
            {
                for(auto& l : _network.get_labels(val, mask, (Query::type_t)(Query::MPLS | (pre.type() & Query::STICKY))))
                {
                    rules.push_back(cpy);
                    rules.back()._pre = l;
                    assert(rules.back()._pre.mask() == 0 || rules.back()._pre.type() != Query::MPLS);
                }
            }
            break;
        }
        case Query::ANYIP:
            mask = 64;
            val = 0;
            // fall through to IP            
        case Query::IP4:
            for(auto& l : _network.get_labels(val, mask, Query::IP4))
            {
                rules.push_back(cpy);
                rules.back()._pre = l;
                assert(rules.back()._pre.mask() == 0 || rules.back()._pre.type() != Query::IP4 || rules.back()._pre == Query::label_t::any_ip4);
            }
            if(pre.type() != Query::ANYIP) break; // fall through to IP6 if any
        case Query::IP6:
            for(auto& l : _network.get_labels(val, mask, Query::IP6))
            {
                rules.push_back(cpy);
                rules.back()._pre = l;
                assert(rules.back()._pre.mask() == 0 || rules.back()._pre.type() != Query::IP6 || rules.back()._pre == Query::label_t::any_ip6);
            }            
            break;
        default:
            throw base_error("Unknown label-type");
        }
    }

    bool NetworkPDAFactory::start_rule(size_t id, nstate_t& s, const RoutingTable::forward_t& forward, const RoutingTable::entry_t& entry, NFA::state_t* destination, std::vector<NetworkPDAFactory::PDAFactory::rule_t>& result)
    {
        rule_t nr;                            
        auto appmode = s._appmode;
        if (!forward._via->is_virtual()) {
            appmode = set_approximation(s, forward);
            if (appmode == std::numeric_limits<int32_t>::max())
                return false;
        }
        bool is_virtual = entry._ingoing != nullptr && entry._ingoing->is_virtual();
        if(_only_mpls_swap && !is_virtual && (
            entry._top_label.type() == Query::INTERFACE ||
            entry._top_label.type() == Query::ANYIP ||
            entry._top_label.type() == Query::IP4 ||
            entry._top_label.type() == Query::IP6))
        {
            auto lb = std::lower_bound(_initial.begin(), _initial.end(), id);
            if(lb == std::end(_initial) || *lb != id) // allow for IP-routing on initial
            {
                return false;            
            }
        }
        if (forward._ops.size() > 0) {
            // check if no-ip-swap is set
            
            if(_only_mpls_swap && !is_virtual && (
               forward._ops[0]._op_label.type() == Query::ANYIP ||
               forward._ops[0]._op_label.type() == Query::IP4 ||
               forward._ops[0]._op_label.type() == Query::IP6))
            {
                return false;
            }

            switch (forward._ops[0]._op) {
            case RoutingTable::POP:
                nr._op = PDA::POP;
                assert(entry._top_label.type() == Query::MPLS || 
                       entry._top_label.type() == Query::STICKY_MPLS);
                break;
            case RoutingTable::PUSH:
                nr._op = PDA::PUSH;
                assert(forward._ops[0]._op_label.type() == Query::MPLS || 
                       forward._ops[0]._op_label.type() == Query::STICKY_MPLS);
                nr._op_label = forward._ops[0]._op_label;
                break;
            case RoutingTable::SWAP:
                nr._op = PDA::SWAP;
                nr._op_label = forward._ops[0]._op_label;
                break;
            }
        }
        else {
            // recheck here; no IP-IP swap.
            if(_only_mpls_swap && !is_virtual && (
                entry._top_label.type() == Query::ANYIP ||
                entry._top_label.type() == Query::IP4 ||
                entry._top_label.type() == Query::IP6))
            {
                return false;
            }
            if(entry._top_label.type() != Query::ANYIP &&
               entry._top_label.type() != Query::ANYMPLS &&
               entry._top_label.type() != Query::ANYSTICKY)
            {
                nr._op = PDA::SWAP;
                nr._op_label = entry._top_label;
                assert(entry._top_label.mask() == 0);
                assert(entry._top_label.type() == Query::MPLS ||
                       entry._top_label.type() == Query::STICKY_MPLS);
            }
            else
            {
                nr._op = PDA::NOOP;
            }
        }
        
        
        if (forward._ops.size() <= 1) {
            std::vector<NFA::state_t*> next{destination};
            if (forward._via->is_virtual())
                next = {s._nfastate};
            else
                NFA::follow_epsilon(next);
            for (auto& n : next) {
                result.emplace_back(nr);
                auto& ar = result.back();
                auto res = add_state(n, forward._via->match(), appmode);
                ar._dest = res.second;
                expand_back(result, entry._top_label);
            }
        }
        else {
            std::vector<NFA::state_t*> next{destination};
            if (forward._via->is_virtual())
                next = {s._nfastate};
            else
                NFA::follow_epsilon(next);
            for (auto& n : next) {
                result.emplace_back(nr);
                auto& ar = result.back();
                auto eid = ((&entry) - s._inf->table().entries().data());
                auto rid = ((&forward) - entry._rules.data());
                auto res = add_state(n, s._inf, appmode, eid, rid, 0);
                ar._dest = res.second;
                expand_back(result, entry._top_label);
            }
        }        
        return true;
    }
    

    std::vector<NetworkPDAFactory::PDAFactory::rule_t> NetworkPDAFactory::rules(size_t id)
    {
        if(_query.approximation() == Query::EXACT ||
           _query.approximation() == Query::DUAL)
        {
            throw base_error("Exact and Dual analysis method not yet supported");
        }
        nstate_t s;
        _states.unpack(id, &s);
        std::vector<NetworkPDAFactory::PDAFactory::rule_t> result;
        if (s._opid < 0) {
            if (s._inf == nullptr) 
                return result;
            // all clean! start pushing.
            for (auto& entry : s._inf->table().entries()) {
                for (auto& forward : entry._rules) {
                    if (forward._via == nullptr || forward._via->target() == nullptr)
                    {
                        continue; // drop/discard/lookup
                    }
                    if(forward._via->is_virtual())
                    {
                        if(!start_rule(id, s, forward, entry, s._nfastate, result))
                            continue;
                    }
                    else
                    {
                        for (auto& e : s._nfastate->_edges) {
                            if (e.empty(_network.all_interfaces().size()))
                            {
                                continue;
                            }
                            auto iid = Query::label_t{Query::INTERFACE, 0, forward._via->global_id()};
                            auto lb = std::lower_bound(e._symbols.begin(), e._symbols.end(), iid);
                            bool found = lb != std::end(e._symbols) && *lb == iid;

                            if (found != (!e._negated)) {
                                continue;
                            }
                            else {
                                if(!start_rule(id, s, forward, entry, e._destination, result))
                                    continue;
                            }
                        }
                    }
                }
            }
        }
        else {
            auto& r = s._inf->table().entries()[s._eid]._rules[s._rid];
            result.emplace_back();
            auto& nr = result.back();
            auto& act = r._ops[s._opid + 1];
            nr._pre = s._inf->table().entries()[s._eid]._top_label;
            for (auto pid = 0; pid <= s._opid; ++pid) {
                switch (r._ops[pid]._op) {
                case RoutingTable::SWAP:
                case RoutingTable::PUSH:
                    nr._pre = r._ops[pid]._op_label;
                    break;
                case RoutingTable::POP:
                default:
                    throw base_error("Unexpected pop!");
                    assert(false);

                }
            }
            switch (act._op) {
            case RoutingTable::POP:
                nr._op = PDA::POP;
                throw base_error("Unexpected pop!");
                assert(false);
                break;
            case RoutingTable::PUSH:
                nr._op = PDA::PUSH;
                nr._op_label = act._op_label;
                break;
            case RoutingTable::SWAP:
                nr._op = PDA::SWAP;
                nr._op_label = act._op_label;
                break;
            }
            if (s._opid + 2 == (int) r._ops.size()) {
                auto res = add_state(s._nfastate, r._via->match());
                nr._dest = res.second;
            }
            else {
                auto res = add_state(s._nfastate, s._inf, s._appmode, s._eid, s._rid, s._opid + 1);
                nr._dest = res.second;
            }
        }
        return result;
    }

    void NetworkPDAFactory::print_trace_rule(std::ostream& stream, const Router* router, const RoutingTable::entry_t& entry, const RoutingTable::forward_t& rule) const {
        stream << "{\"pre\": ";
        if(entry._top_label.type() == Query::INTERFACE)
        {
            assert(false);
        }
        else
        {
            stream << "\"" << entry._top_label;
            if(_network.is_service_label(entry._top_label))
                stream << "^";
            stream << "\"";
        }
        stream << " ,\"rule\": ";
        rule.print_json(stream, false);
        stream << "}";
    }

    bool NetworkPDAFactory::add_interfaces(std::unordered_set<const Interface*>& disabled, std::unordered_set<const Interface*>& active, const RoutingTable::entry_t& entry, 
                                           const RoutingTable::forward_t& fwd) const
    {
        auto* inf = fwd._via;
        if(disabled.count(inf) > 0) 
        {
            return false; // should be down!
        }
        // find all rules with a "covered" pre but lower weight.
        // these must have been disabled!
        if(fwd._weight == 0)
            return true;
        std::unordered_set<const Interface*> tmp = disabled;
        bool brk = false;
        for(auto& alt_ent : inf->table().entries())
        {
            if(alt_ent._top_label.overlaps(entry._top_label))
            {
                for(auto& alt_rule : alt_ent._rules)
                {
                    if(alt_rule._weight < fwd._weight)
                    {
                        if(active.count(alt_rule._via) > 0) return false;
                        tmp.insert(alt_rule._via);
                        if(tmp.size() > (uint32_t)_query.number_of_failures())
                        {
                            return false;
                        }
                        brk = true;
                        break;
                    }
                }
            }
            if(brk)
                break;
        }
        if(tmp.size() > (uint32_t)_query.number_of_failures())
        {
            return false;
        }
        else
        {
            disabled.swap(tmp);
            active.insert(fwd._via);
            return true;
        }
    }

    bool NetworkPDAFactory::concreterize_trace(std::ostream& stream, const std::vector<PDA::tracestate_t>& trace, 
                                               std::vector<const RoutingTable::entry_t*>& entries, 
                                               std::vector<const RoutingTable::forward_t*>& rules)
    {
        std::unordered_set<const Interface*> disabled, active;

        for(size_t sno = 0; sno < trace.size(); ++sno)
        {
            auto& step = trace[sno];
            if(step._pdastate < _num_pda_states)
            {
                // handle, lookup right states
                nstate_t s;
                _states.unpack(step._pdastate, &s);
                if(s._opid >= 0)
                {
                    // Skip, we are just doing a bunch of ops here, printed elsewhere.
                }
                else
                {
                    if(sno != trace.size() - 1 && trace[sno + 1]._pdastate < _num_pda_states && !step._stack.empty())
                    {
                        // peek at next element, we want to write the ops here
                        nstate_t next;
                        _states.unpack(trace[sno + 1]._pdastate, &next);
                        if(next._opid != -1)
                        {
                            // we get the rule we use, print
                            auto& entry = next._inf->table().entries()[next._eid];
                            if(!add_interfaces(disabled, active, entry, entry._rules[next._rid]))
                            {
                                return false;
                            }
                            rules.push_back(&entry._rules[next._rid]);
                            entries.push_back(&entry);
                        }
                        else 
                        {
                            // we have to guess which rule we used!
                            // run through the rules and find a match!
                            auto& nstep = trace[sno + 1];
                            bool found = false;
                            for(auto& entry : s._inf->table().entries())
                            {
                                if(found) break;
                                if(!entry._top_label.overlaps(step._stack.front()))
                                    continue; // not matching on pre
                                for(auto& r : entry._rules)
                                {
                                    bool ok = false;
                                    switch(_query.approximation())
                                    {
                                    case Query::UNDER:
                                        assert(next._appmode >= s._appmode);
                                        ok = ((ssize_t)r._weight) == (next._appmode - s._appmode);
                                        break;
                                    case Query::OVER:
                                        ok = ((ssize_t)r._weight) <= _query.number_of_failures();
                                        break;
                                    case Query::DUAL:
                                        throw base_error("Tracing for DUAL not yet implemented");
                                    case Query::EXACT:
                                        throw base_error("Tracing for EXACT not yet implemented");
                                    }
                                    if(ok) // TODO, fix for approximations here!
                                    {
                                        if(r._ops.size() > 1) continue; // would have been handled in other case

                                        if(r._via && r._via->match() == next._inf)
                                        {
                                            if(r._ops.empty() || r._ops[0]._op == RoutingTable::SWAP)
                                            {
                                                if(step._stack.size() == nstep._stack.size())
                                                {
                                                    if(!r._ops.empty()) 
                                                    {
                                                        assert(r._ops[0]._op == RoutingTable::SWAP);
                                                        if(nstep._stack.front() != r._ops[0]._op_label)
                                                            continue;
                                                    }
                                                    else if(!entry._top_label.overlaps(nstep._stack.front()))
                                                    {
                                                        continue;
                                                    }
                                                    if(!add_interfaces(disabled, active, entry, r))
                                                        continue;
                                                    rules.push_back(&r);
                                                    entries.push_back(&entry);
                                                    found = true;
                                                    break;
                                                }
                                            }
                                            else
                                            {
                                                assert(r._ops.size() == 1);
                                                assert(r._ops[0]._op == RoutingTable::PUSH || r._ops[0]._op == RoutingTable::POP);
                                                if(r._ops[0]._op == RoutingTable::POP && nstep._stack.size() == step._stack.size() - 1)
                                                {
                                                    if(!add_interfaces(disabled, active, entry, r))
                                                        continue;
                                                    rules.push_back(&r);
                                                    entries.push_back(&entry);
                                                    found = true;
                                                    break;
                                                }
                                                else if(r._ops[0]._op == RoutingTable::PUSH && nstep._stack.size() == step._stack.size() + 1 &&
                                                        r._ops[0]._op_label == nstep._stack.front())
                                                {
                                                    if(!add_interfaces(disabled, active, entry, r))
                                                        continue;

                                                    rules.push_back(&r);
                                                    entries.push_back(&entry);
                                                    found = true;
                                                    break;                                                        
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                            
                            // check if we violate the soundness of the network
                            if(!found)
                            {
                                stream << "{\"pre\":\"error\"}";
                                return false;
                            }
                        }
                    }
                }
            }
            else
            {
                // construction, destruction, BORRING!
                // SKIP
            }
        }    
        return true;
    }
    
    void NetworkPDAFactory::write_concrete_trace(std::ostream& stream, const std::vector<PDA::tracestate_t>& trace, 
                                                    std::vector<const RoutingTable::entry_t*>& entries, 
                                                    std::vector<const RoutingTable::forward_t*>& rules)
    {
        bool first = true;
        size_t cnt = 0;
        for(size_t sno = 0; sno < trace.size(); ++sno)
        {
            auto& step = trace[sno];
            if(step._pdastate < _num_pda_states)
            {
                nstate_t s;
                _states.unpack(step._pdastate, &s);
                if(s._opid >= 0)
                {
                    // Skip, we are just doing a bunch of ops here, printed elsewhere.
                }
                else
                {
                    if(!first)
                        stream << ",\n";
                    stream << "\t\t\t\t{\"router\": ";
                    if(s._inf)
                        stream << "\"" << s._inf->source()->name() << "\"";
                    else
                        stream << "null";
                    stream << ", \"stack\": [";
                    bool first_symbol = true;
                    for(auto& symbol : step._stack)
                    {
                        if(!first_symbol)
                            stream << ", ";
                        stream << "\"" << symbol;
                        if(_network.is_service_label(symbol))
                            stream << "^";
                        stream << "\"";
                        first_symbol = false;
                    }                    
                    stream << "]}";
                    if(cnt < entries.size())
                    {
                        stream << ",\n\t\t\t\t";
                        print_trace_rule(stream, s._inf->source(), *entries[cnt], *rules[cnt]);                        
                        ++cnt;
                    }
                    first = false;
                }
            }
        }        
    }
    
    void NetworkPDAFactory::substitute_wildcards(std::vector<PDA::tracestate_t>& trace, 
                                                    std::vector<const RoutingTable::entry_t*>& entries, 
                                                    std::vector<const RoutingTable::forward_t*>& rules) 
    {
        size_t cnt = 0;
        for(size_t sno = 0; sno < trace.size(); ++sno)
        {
            auto& step = trace[sno];
            if(step._pdastate < _num_pda_states)
            {
                nstate_t s;
                _states.unpack(step._pdastate, &s);
                if(s._opid >= 0)
                {
                    // Skip, we are just doing a bunch of ops here, printed elsewhere.
                }
                else
                {
                    if(cnt < entries.size())
                    {
                        Query::label_t concrete;
                        bool some = false;

                        if(!step._stack.empty())
                        {
                            //            ANYMPLS = 1, ANYIP = 2, IP4 = 4, IP6 = 8, MPLS = 16, STICKY = 32, INTERFACE = 64, NONE = 128, ANYSTICKY = ANYMPLS | STICKY, STICKY_MPLS = MPLS | STICKY
                            switch(step._stack.front().type())
                            {
                            case Query::ANYMPLS:
                            case Query::ANYSTICKY:
                            case Query::ANYIP:
                            case Query::IP4:
                            case Query::IP6:
                                if(entries[cnt]->_top_label != Query::label_t::any_ip &&
                                   entries[cnt]->_top_label.mask() < step._stack.front().mask())
                                {
                                    concrete = entries[cnt]->_top_label;
                                    some = true;
                                }
                                break;
                            default:
                                break;
                            }
                        }
                        if(some)
                        {
                            for(size_t upd = 0; upd <= sno; ++upd)
                            {
                                for(auto& s : trace[upd]._stack)
                                {
                                    if(s.type() == Query::ANYIP ||
                                       s.type() == Query::IP4 ||
                                       s.type() == Query::IP6 ||
                                       s.type() == Query::ANYSTICKY ||
                                       s.type() == Query::ANYMPLS)
                                    {
                                        s = concrete;
                                        break;
                                    }
                                }
                            }
                        }
                        ++cnt;
                    }
                }
            }
        }        
    }

    bool NetworkPDAFactory::write_json_trace(std::ostream& stream, std::vector<PDA::tracestate_t>& trace)
    {

        std::vector<const RoutingTable::entry_t*> entries;
        std::vector<const RoutingTable::forward_t*> rules;
        
        if(!concreterize_trace(stream, trace, entries, rules))
        {
            return false;
        }
        
        // Fix trace
        substitute_wildcards(trace, entries, rules);

        // Do the printing
        write_concrete_trace(stream, trace, entries, rules);
        return true;
    }
    
    std::function<void(std::ostream&, const Query::label_t&) > NetworkPDAFactory::label_writer() const
    {
        return [](std::ostream& s, const Query::label_t & label)
        {
            RoutingTable::entry_t::print_label(label, s, false);
        };
    }
    
}
