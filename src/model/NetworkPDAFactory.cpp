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
        Router* nr = nullptr;
        add_state(ns, nr);
        construct_initial();
        _path.compile();
    }

    void NetworkPDAFactory::construct_initial()
    {
        // there is potential for some serious pruning here!
        auto add_initial = [&, this](NFA::state_t* state, const Router * router)
        {
            std::vector<NFA::state_t*> next{state};
            NFA::follow_epsilon(next);
            for (auto& n : next) {
                auto res = add_state(n, router);
                if (res.first)
                    _initial.push_back(res.second);
            }
        };

        for (auto i : _path.initial()) {
            // the path is one behind, we are coming from an unknown router via an OK interface
            // i.e. we have to move straight to the next state
            for (auto& e : i->_edges) {
                if (e.wildcard(_network.all_interfaces().size())) {
                    for (size_t r = 0; r < _network.size(); ++r) {
                        add_initial(e._destination, _network.get_router(r));
                    }
                }
                else if (!e._negated) {
                    for (auto s : e._symbols) {
                        Interface* interface = reinterpret_cast<Interface*> (s);
                        add_initial(e._destination, interface->target());
                    }
                }
                else {
                    for (auto inf : _network.all_interfaces()) {
                        Query::label_t iid = reinterpret_cast<Query::label_t> (inf);
                        auto lb = std::lower_bound(e._symbols.begin(), e._symbols.end(), iid);
                        if (lb == std::end(e._symbols) || *lb != iid) {
                            add_initial(e._destination, inf->target());
                        }
                    }
                }
            }
        }
    }

    std::pair<bool, size_t> NetworkPDAFactory::add_state(NFA::state_t* state, const Router* router, int32_t mode, int32_t table, int32_t eid, int32_t fid, int32_t op)
    {
        nstate_t ns;
        ns._appmode = mode;
        ns._nfastate = state;
        ns._opid = op;
        ns._router = router;
        ns._rid = fid;
        ns._tid = table;
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
/*                if(res.first) std::cerr << "## NEW " << std::endl;
                std::string rn;
                if(router)
                    rn = router->name();
                else
                    rn = "SINK";
                std::cerr << "ADDED STATE " << state << " R " << rn << " M" << mode << " T" << table << " F" << fid << " O" << op << std::endl;
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

    std::vector<NetworkPDAFactory::PDAFactory::rule_t> NetworkPDAFactory::rules(size_t id)
    {
        nstate_t s;
        _states.unpack(id, (unsigned char*) &s);
        std::vector<NetworkPDAFactory::PDAFactory::rule_t> result;
        if (s._opid == -1) {
            if (s._router == nullptr) return result;
            // all clean! start pushing.
            for (auto& table : s._router->tables()) {
                for (auto& entry : table.entries()) {
                    for (auto& forward : entry._rules) {
                        if (forward._via == nullptr) continue; // drop/discard/lookup
                        for (auto& e : s._nfastate->_edges) {
                            if (e.empty(_network.all_interfaces().size()))
                                continue;
                            auto iid = reinterpret_cast<Query::label_t> (forward._via);
                            auto lb = std::lower_bound(e._symbols.begin(), e._symbols.end(), iid);
                            bool found = lb != std::end(e._symbols) && *lb == iid;

                            if (found != (!e._negated)) {
                                /*                                auto name = forward._via->source()->interface_name( forward._via->id());
                                                                std::cerr << "IID " << iid << std::endl;
                                                                std::cerr << "COLD NOT FIND " <<  forward._via->source()->name() << "." << name.get() << " (" << forward._via << ") " << " IN [\n";
                                                                if(e._negated) std::cerr << "NEG" << std::endl;
                                                                for(auto& n : e._symbols)
                                                                {
                                                                    Interface* inf = reinterpret_cast<Interface*>(n);
                                                                    auto rn = inf->source()->interface_name(inf->id());
                                                                    std::cerr << "\t" << inf->source()->name() << "." << rn.get() << " (" << inf << ")" << reinterpret_cast<ssize_t>(inf) << std::endl;
                                                                }
                                                                std::cerr << "];\n";
                                                                std::cerr << std::endl;*/
                            }
                            else {
                                rule_t nr;
                                /*
                                std::stringstream ss;
                                auto name = forward._via->source()->interface_name( forward._via->id());
                                ss << s._router->name() << "." << name.get() << " -- [";
                                RoutingTable::entry_t::print_label(entry._top_label, ss, false);
                                ss << "| ";
                                for(auto& o : forward._ops)
                                {
                                    o.print_json(ss, false);
                                    ss << " ";
                                }
                                ss << "]";
                                ss << " --> ";
                                if(forward._via != nullptr && forward._via->target() != nullptr) ss << forward._via->target()->name();
                                else ss << "SINK";
                                nr._verbose = ss.str();*/
                                nr._pre = entry._top_label;
                                auto appmode = s._appmode;
                                if (!forward._via->is_virtual()) {
                                    appmode = set_approximation(s, forward);
                                    if (appmode == std::numeric_limits<int32_t>::max())
                                        continue;
                                }
                                if (forward._ops.size() > 0) {
                                    switch (forward._ops[0]._op) {
                                    case RoutingTable::POP:
                                        nr._op = POP;
                                        assert(!entry.is_interface());
                                        break;
                                    case RoutingTable::PUSH:
                                        if (entry.is_interface()) {
                                            nr._op = SWAP;
                                            nr._op_label = forward._ops[0]._op_label;
                                        }
                                        else {
                                            nr._op = PUSH;
                                            assert(forward._ops[0]._op_label >= 0);
                                            nr._op_label = forward._ops[0]._op_label;
                                        }
                                        break;
                                    case RoutingTable::SWAP:
                                        nr._op = SWAP;
                                        nr._op_label = forward._ops[0]._op_label;
                                        assert(!entry.is_interface());
                                        break;
                                    }
                                }
                                else {
/*                                    std::stringstream ss;
                                    if(nr._pre < 0)
                                    {
                                        auto ino = (nr._pre + 1) * -1;
                                        const Interface* inf = _network.all_interfaces()[ino];
                                        ss << "Cannot forward interface-label on ";
                                        if(inf->source())
                                            ss << inf->source()->name();
                                        else
                                            ss << "SINK";
                                        ss << "\n";
                                        ss << "Routing-table: " << table.name() << "\n";
                                        if(inf->source())
                                        {
                                            ss << "\n";
                                            auto iname = inf->source()->interface_name(inf->id());
                                            ss << "Interface : ";
                                            inf->print_json(ss, iname.get());
                                        }
                                        else if(inf->target())
                                        {
                                            std::cerr << "TARGET " << inf->target()->name() << "\n";
                                        }
                                        entry.print_json(ss);
                                        throw base_error(ss.str());
                                    }*/
                                    // Probably shouldnt happen?
                                    nr._op = NOOP;
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
                                        auto res = add_state(n, forward._via->target(), appmode);
                                        ar._dest = res.second;
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
                                        auto tid = ((&table) - s._router->tables().data());
                                        auto eid = ((&entry) - table.entries().data());
                                        auto rid = ((&forward) - entry._rules.data());
                                        auto res = add_state(n, s._router, appmode, tid, eid, rid, 0);
                                        ar._dest = res.second;
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
        else {
            auto& r = s._router->tables()[s._tid].entries()[s._eid]._rules[s._rid];
            result.emplace_back();
            auto& nr = result.back();
            auto& act = r._ops[s._opid + 1];
            nr._pre = s._router->tables()[s._tid].entries()[s._eid]._top_label;
            for (auto pid = 0; pid <= s._opid; ++pid) {
                switch (r._ops[pid]._op) {
                case PUSH:
                case SWAP:
                    nr._pre = r._ops[pid]._op_label;
                    break;
                case POP:
                default:
                    throw base_error("Unexpected pop!");
                    assert(false);

                }
            }
            switch (act._op) {
            case RoutingTable::POP:
                nr._op = POP;
                throw base_error("Unexpected pop!");
                assert(false);
                break;
            case RoutingTable::PUSH:
                nr._op = PUSH;
                nr._op_label = act._op_label;
                break;
            case RoutingTable::SWAP:
                nr._op = SWAP;
                nr._op_label = act._op_label;
                break;
            }
            if (s._opid + 2 == (int) r._ops.size()) {
                auto res = add_state(s._nfastate, r._via->target());
                nr._dest = res.second;
            }
            else {
                auto res = add_state(s._nfastate, s._router, s._appmode, s._tid, s._eid, s._rid, s._opid + 1);
                nr._dest = res.second;
            }
        }
        return result;
    }

    void NetworkPDAFactory::print_trace_rule(std::ostream& stream, const Router* router, const RoutingTable::entry_t& entry, const RoutingTable::forward_t& rule) const {
        stream << "{\"pre\":";
        if(entry._top_label < 0)
        {
            auto inf = _network.all_interfaces()[entry.as_interface()];
            auto iname = inf->source()->interface_name(inf->id());
            stream << "\"" << iname.get() << "\"";
        }
        else
        {
            RoutingTable::entry_t::print_label(entry._top_label, stream, true);
        }
        stream << ",rule\":";
        rule.print_json(stream);
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
                if(s._opid != -1)
                {
                    // Skip, we are just doing a bunch of ops here, printed elsewhere.
                }
                else
                {
                    if(!first)
                        stream << ",\n";
                    stream << "\t\t\t{\"router\":\"" << s._router->name() << "\",\"stack\":[";
                    bool first_symbol = true;
                    for(auto& symbol : step._stack)
                    {
                        if(!first_symbol)
                            stream << ",";
                        if(symbol.first)
                            stream << "\".\"";
                        else 
                            RoutingTable::entry_t::print_label(symbol.second, stream, true);
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
                            auto& entry = next._router->tables()[next._tid].entries()[next._eid];
                            print_trace_rule(stream, next._router, entry, entry._rules[next._rid]);
                        }
                        else 
                        {
                            // we have to guess which rule we used!
                            // run through the rules and find a match!
                            auto& nstep = trace[sno + 1];
                            bool found = false;
                            for(auto& table : s._router->tables())
                            {
                                if(found) break;
                                for(auto& entry : table.entries())
                                {
                                    if(found) break;
                                    if(entry.is_default())
                                        continue;
                                    if(!step._stack.front().first && step._stack.front().second != entry._top_label)
                                        continue; // not matching on pre
                                    for(auto& r : entry._rules)
                                    {
                                        if(((ssize_t)r._weight) <= _query.number_of_failures()) // TODO, fix for approximations here!
                                        {
                                            if(r._ops.size() > 1) continue; // would have been handled in other case

                                            if(r._via && r._via->target() == next._router)
                                            {
                                                if(r._ops.empty() || r._ops[0]._op == RoutingTable::SWAP)
                                                {
                                                    if(step._stack.size() == nstep._stack.size() &&
                                                       nstep._stack.front().second == (r._ops.empty() ? entry._top_label : r._ops[0]._op_label) &&
                                                       !nstep._stack.front().first)
                                                    {
                                                        print_trace_rule(stream, s._router, entry, r);
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
                                                        print_trace_rule(stream, s._router, entry, r);
                                                        found = true;
                                                        break;
                                                    }
                                                    else if(r._ops[0]._op == RoutingTable::PUSH && nstep._stack.size() == step._stack.size() + 1 &&
                                                            r._ops[0]._op_label == nstep._stack.front().second && !nstep._stack.front().first)
                                                    {
                                                        print_trace_rule(stream, s._router, entry, r);
                                                        found = true;
                                                        break;                                                        
                                                    }
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
                auto l = strtoll(from, (char**)&to, 10);
                auto rl = Query::label_t{l + 1};
                if(_all_labels.count(rl) != 1)
                {
                    std::stringstream e;
                    e << "Unknown label: " << l;
                    throw base_error(e.str());
                }
                return rl;
            }
            else if(*from == 'i')
            {
                auto i = strtoll(from, (char**)&to, 10);
                if((size_t)i >= _network.all_interfaces().size())
                {
                    std::stringstream e;
                    e << "Unknown interface ID: " << i;
                    throw base_error(e.str());
                }
                return Query::label_t{(i+1)*-1};
            }
            else
            {
                throw base_error("Unknown label");
            }
            return Query::label_t{0};
        };
    }
}
