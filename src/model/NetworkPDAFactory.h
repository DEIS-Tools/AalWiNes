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
 * File:   NetworkPDAFactory.h
 * Author: Peter G. Jensen <root@petergjoel.dk>
 *
 * Created on July 19, 2019, 6:26 PM
 */

#ifndef NETWORKPDA_H
#define NETWORKPDA_H

#include "Query.h"
#include "Network.h"
#include "pdaaal/model/PDAFactory.h"


namespace mpls2pda
{
    class NetworkPDAFactory : public pdaaal::PDAFactory<Query::label_t> {
        using label_t = Query::label_t;
        using NFA = pdaaal::NFA<label_t>;
        using PDAFactory = pdaaal::PDAFactory<label_t>;
        using PDA = pdaaal::TypedPDA<label_t>;
    private:        
        struct nstate_t {
            bool _initial = false;
            int32_t _appmode = 0; // mode of approximation
            int32_t _opid = -1; // which operation is the first in the rule (-1=cleaned up).
            int32_t _eid = 0; // which entry we are going for
            int32_t _rid = 0; // which rule in that entry
            NFA::state_t* _nfastate = nullptr;
            const Interface* _inf = nullptr;
        } __attribute__((packed)); // packed is needed to make this work directly with ptries
    public:
        NetworkPDAFactory(Query& q, Network& network, bool only_mpls_swap);
        
        std::function<void(std::ostream&, const Query::label_t&) > label_writer() const;
        
        bool write_json_trace(std::ostream& stream, std::vector<PDA::tracestate_t>& trace);
        
        
    protected:
        const std::vector<size_t>& initial() override;
        bool empty_accept() const override;
        bool accepting(size_t) override;
        std::vector<rule_t> rules(size_t ) override;
        void expand_back(std::vector<rule_t>& rules, const Query::label_t& pre);
        bool start_rule(size_t id, nstate_t& s, const RoutingTable::forward_t& fwd, const RoutingTable::entry_t& entry, NFA::state_t* destination, std::vector<NetworkPDAFactory::PDAFactory::rule_t>& result);
    private:
        bool add_interfaces(std::unordered_set<const Interface*>& disabled, std::unordered_set<const Interface*>& active, const RoutingTable::entry_t& entry, const RoutingTable::forward_t& fwd) const;
        void print_trace_rule(std::ostream& stream, const Router* router, const RoutingTable::entry_t& entry, const RoutingTable::forward_t& rule) const;

        void construct_initial();
        std::pair<bool,size_t> add_state(NFA::state_t* state, const Interface* router, int32_t mode = 0, int32_t eid = 0, int32_t fid = 0, int32_t op = -1, bool initial = false);
        int32_t set_approximation(const nstate_t& state, const RoutingTable::forward_t& forward);
        
        bool concreterize_trace(std::ostream& stream, const std::vector<PDA::tracestate_t>& trace, 
                                                std::vector<const RoutingTable::entry_t*>& entries, 
                                                std::vector<const RoutingTable::forward_t*>& rules);
        void write_concrete_trace(std::ostream& stream, const std::vector<PDA::tracestate_t>& trace, 
                                                std::vector<const RoutingTable::entry_t*>& entries, 
                                                std::vector<const RoutingTable::forward_t*>& rules);
        void substitute_wildcards(std::vector<PDA::tracestate_t>& trace, 
                                                std::vector<const RoutingTable::entry_t*>& entries, 
                                                std::vector<const RoutingTable::forward_t*>& rules);
        
        Network& _network;
        Query& _query;
        NFA& _path;
        std::vector<size_t> _initial;
        ptrie::map<bool> _states;
        bool _only_mpls_swap = false;
    };
}

#endif /* NETWORKPDA_H */

