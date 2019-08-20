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
 *  Copyright Peter G. Jensen, all rights reserved.
 */

/* 
 * File:   NetworkPDAFactory.cpp
 * Author: Peter G. Jensen <root@petergjoel.dk>
 * 
 * Created on July 19, 2019, 6:26 PM
 */

#include "NetworkPDAFactory.h"
#include "pdaaal/model/PDAFactory.h"

namespace mpls2pda
{

    NetworkPDAFactory::NetworkPDAFactory(Query& query, Network& network)
    : PDAFactory(query.construction(), query.destruction(), network.all_labels()), _network(network), _query(query), _path(query.path())
    {
        // add special NULL state initially
        NFA::state_t* ns = nullptr;
        Interface* nr = nullptr;
        add_state(ns, nr);
        construct_initial();
        _path.compile();
    }

    void NetworkPDAFactory::construct_initial()
    {
        // there is potential for some serious pruning here!
        auto add_initial = [&, this](NFA::state_t* state, const Interface * inf)
        {
            std::vector<NFA::state_t*> next{state};
            NFA::follow_epsilon(next);
            for (auto& n : next) {
                auto res = add_state(n, inf, 0, 0, 0, -1);
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
                        assert(s._type == Query::INTERFACE);
                        auto inf = _network.all_interfaces()[s._value];
                        add_initial(e._destination, inf->match());
                    }
                }
                else {
                    for (auto inf : _network.all_interfaces()) {
                        auto iid = Query::label_t{Query::INTERFACE, 0, inf->global_id()};
                        auto lb = std::lower_bound(e._symbols.begin(), e._symbols.end(), iid);
                        if (lb == std::end(e._symbols) || *lb != iid) {
                            add_initial(e._destination, inf->match());
                        }
                    }
                }
            }
        }
    }

    std::pair<bool, size_t> NetworkPDAFactory::add_state(NFA::state_t* state, const Interface* inf, int32_t mode, int32_t eid, int32_t fid, int32_t op)
    {
        nstate_t ns;
        ns._appmode = mode;
        ns._nfastate = state;
        ns._opid = op;
        ns._inf = inf;
        ns._rid = fid;
        ns._eid = eid;
        auto res = _states.insert((unsigned char*) &ns, sizeof (ns));
        if (res.first) {
            if(res.second != 0)
            {
                // don't check null-state
                auto& d = _states.get_data(res.second);
                d = op == -1 ? state->_accepting : false;
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

        default:
            return err;
        }
    }

    void NetworkPDAFactory::expand_back(std::vector<rule_t>& rules, const Query::label_t& pre)
    {
        rule_t cpy;
        std::swap(rules.back(), cpy);
        rules.pop_back();
        auto val = pre._value;
        auto mask = pre._mask;
        
        switch(pre._type)
        {
        case Query::ANYMPLS:
            mask = 64;
            val = 0;
            // fall through to MPLS
        case Query::MPLS:
        {
            if(mask == 0)
            {
                rules.push_back(cpy);
                rules.back()._pre = pre;
                rules.push_back(cpy);
                rules.back()._pre = Query::label_t::any_mpls;
            }
            else
            {
                for(auto& l : _network.get_labels(val, mask, Query::MPLS))
                {
                    rules.push_back(cpy);
                    rules.back()._pre = l;
                    assert(rules.back()._pre._mask == 0 || rules.back()._pre._type != Query::MPLS);
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
                assert(rules.back()._pre._mask == 0 || rules.back()._pre._type != Query::IP4 || rules.back()._pre == Query::label_t::any_ip4);
            }
            if(pre._type != Query::ANYIP) break; // fall through to IP6 if any
        case Query::IP6:
            for(auto& l : _network.get_labels(val, mask, Query::IP6))
            {
                rules.push_back(cpy);
                rules.back()._pre = l;
                assert(rules.back()._pre._mask == 0 || rules.back()._pre._type != Query::IP6 || rules.back()._pre == Query::label_t::any_ip6);
            }            
            break;
        default:
            throw base_error("Unknown label-type");
        }
    }


    std::vector<NetworkPDAFactory::PDAFactory::rule_t> NetworkPDAFactory::rules(size_t id)
    {
        if(_query.approximation() == Query::EXACT)
        {
            throw base_error("Exact analysis method not yet supported");
        }
        nstate_t s;
        _states.unpack(id, (unsigned char*) &s);
        std::vector<NetworkPDAFactory::PDAFactory::rule_t> result;
        if (s._opid < 0) {
            if (s._inf == nullptr) return result;
            // all clean! start pushing.
            for (auto& entry : s._inf->table().entries()) {
                for (auto& forward : entry._rules) {
                    if (forward._via == nullptr || forward._via->target() == nullptr)
                    {
                        continue; // drop/discard/lookup
                    }
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
                            rule_t nr;                            
                            auto appmode = s._appmode;
                            if (!forward._via->is_virtual()) {
                                appmode = set_approximation(s, forward);
                                if (appmode == std::numeric_limits<int32_t>::max())
                                    continue;
                            }
                            if (forward._ops.size() > 0) {
                                switch (forward._ops[0]._op) {
                                case RoutingTable::POP:
                                    nr._op = PDA::POP;
                                    assert(entry._top_label._type == Query::MPLS);
                                    break;
                                case RoutingTable::PUSH:
                                    nr._op = PDA::PUSH;
                                    assert(forward._ops[0]._op_label._type == Query::MPLS);
                                    nr._op_label = forward._ops[0]._op_label;
                                    break;
                                case RoutingTable::SWAP:
                                    nr._op = PDA::SWAP;
                                    nr._op_label = forward._ops[0]._op_label;
                                    break;
                                }
                            }
                            else {
                                if(entry._top_label._type != Query::ANYIP &&
                                   entry._top_label._type != Query::ANYMPLS)
                                {
                                    nr._op = PDA::SWAP;
                                    nr._op_label = entry._top_label;
                                    assert(entry._top_label._mask == 0);
                                    assert(entry._top_label._type == Query::MPLS);
                                }
                                else
                                {
                                    nr._op = PDA::NOOP;
                                }
                            }
                            if (forward._ops.size() <= 1) {
                                std::vector<NFA::state_t*> next{e._destination};
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
                                std::vector<NFA::state_t*> next{e._destination};
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
                auto res = add_state(s._nfastate, r._via);
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
        stream << "{\"pre\":";
        if(entry._top_label._type == Query::INTERFACE)
        {
            assert(false);
        }
        else
        {
            stream << "\"" << entry._top_label << "\"";
        }
        stream << ",\"rule\":";
        rule.print_json(stream, false);
        stream << "}";
    }

    void NetworkPDAFactory::write_json_trace(std::ostream& stream, std::vector<PDA::tracestate_t>&& trace)
    {
        bool first = true;
        for(size_t sno = 0; sno < trace.size(); ++sno)
        {
            auto& step = trace[sno];
            if(step._pdastate < _num_pda_states)
            {
                // handle, lookup right states
                nstate_t s;
                _states.unpack(step._pdastate, (unsigned char*) &s);
                if(s._opid >= 0)
                {
                    // Skip, we are just doing a bunch of ops here, printed elsewhere.
                }
                else
                {
                    if(!first)
                        stream << ",\n";
                    stream << "\t\t\t{\"router\":\"" << s._inf->source()->name() << "\",\"stack\":[";
                    bool first_symbol = true;
                    for(auto& symbol : step._stack)
                    {
                        if(!first_symbol)
                            stream << ",";
                        stream << "\"" << symbol << "\"";
                        first_symbol = false;
                    }                    
                    stream << "]}";

                    if(sno != trace.size() - 1 && trace[sno + 1]._pdastate < _num_pda_states && !step._stack.empty())
                    {
                        stream << ",\n\t\t\t";
                        // peek at next element, we want to write the ops here
                        nstate_t next;
                        _states.unpack(trace[sno + 1]._pdastate, (unsigned char*)&next);
                        if(next._opid != -1)
                        {
                            // we get the rule we use, print
                            auto& entry = next._inf->table().entries()[next._eid];
                            print_trace_rule(stream, next._inf->source(), entry, entry._rules[next._rid]);
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
                                                    print_trace_rule(stream, s._inf->source(), entry, r);
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
                                                    print_trace_rule(stream, s._inf->source(), entry, r);
                                                    found = true;
                                                    break;
                                                }
                                                else if(r._ops[0]._op == RoutingTable::PUSH && nstep._stack.size() == step._stack.size() + 1 &&
                                                        r._ops[0]._op_label == nstep._stack.front())
                                                {
                                                    print_trace_rule(stream, s._inf->source(), entry, r);
                                                    found = true;
                                                    break;                                                        
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                            if(!found)
                            {
                                stream << "{\"pre\":\"unknown\"}";
                            }
                        }
                    }
                    first = false;                    
                }
            }
            else
            {
                // construction, destruction, BORRING!
                // SKIP
            }
        }
    }
    
    std::function<void(std::ostream&, const Query::label_t&) > NetworkPDAFactory::label_writer() const
    {
        return [](std::ostream& s, const Query::label_t & label)
        {
            RoutingTable::entry_t::print_label(label, s, false);
        };
    }
    
    std::function<Query::label_t(const char*, const char*) > NetworkPDAFactory::label_reader() const
    {
        return [this](const char* from, const char* to)
        {
            if(to <= from)
                throw base_error("Cannot parse empty string");
            if(*from == 'l')
            {
                ++from;
                auto l = strtoll(from, (char**)&to, 16);
                auto rl = Query::label_t(Query::MPLS, 0, static_cast<uint64_t>(l));
                if(_all_labels.count(rl) != 1)
                {
                    std::stringstream e;
                    e << "Unknown label: " << l;
                    throw base_error(e.str());
                }
                assert(rl._type != Query::NONE);
                return rl;
            }
            else if(*from == 'i')
            {
                Query::label_t res;
                if(*(from + 2) == '4')
                    res = Query::label_t::any_ip4;
                else
                    res = Query::label_t::any_ip6;
                assert(res._type != Query::NONE);
                from = from + 3;
                auto ip = strtoll(from, (char**)&to, 16);
                while(*from != 'M') ++from;
                ++from;
                auto mask = strtoll(from, (char**)&to, 16);
                res.set_value(ip, mask);   
                assert(res._type != Query::NONE);
                return res;
            }
            else if(*from == 'a')
            {
                if(*(from + 1) == 'm')
                    return Query::label_t::any_mpls;
                else 
                    return Query::label_t::any_ip;
            }
            else
            {
                throw base_error("Unknown label");
            }
            return Query::label_t{};
        };
    }
}
