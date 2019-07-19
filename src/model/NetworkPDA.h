/*
 *  Copyright Peter G. Jensen, all rights reserved.
 */

/* 
 * File:   NetworkPDA.h
 * Author: Peter G. Jensen <root@petergjoel.dk>
 *
 * Created on July 19, 2019, 6:26 PM
 */

#ifndef NETWORKPDA_H
#define NETWORKPDA_H

#include "Query.h"
#include "Network.h"
#include "pdaaal/model/PDA.h"


namespace mpls2pda
{
    class NetworkPDA : public pdaaal::PDA<Query::label_t> {
        using label_t = Query::label_t;
        using NFA = pdaaal::NFA<label_t>;
        using PDA = pdaaal::PDA<label_t>;
    private:        
        struct nstate_t {
            int32_t _appmode = 0; // mode of approximation
            int32_t _opid = -1; // which operation is the first in the rule (-1=cleaned up).
            int32_t _tid = 0;
            int32_t _eid = 0;
            int32_t _rid = 0;
            NFA::state_t* _nfastate;
            const Router* _router;
            std::tuple<uint32_t,uint32_t,uint32_t> op2table() const;
            void setop(uint32_t,uint32_t,uint32_t);
        };
    public:
        NetworkPDA(Query& q, Network& network);
        const std::vector<size_t>& initial() override;
        bool empty_accept() const override;
        bool accepting(size_t) override;
        std::vector<rule_t> rules(size_t ) override;

    private:
        void construct_initial();
        std::pair<bool,size_t> add_state(NFA::state_t* state, const Router* router, int32_t mode = 0, int32_t tid = 0, int32_t eid = 0, int32_t fid = 0, int32_t op = -1);
        Network& _network;
        Query& _query;
        NFA& _path;
        std::vector<size_t> _initial;
        ptrie::map<bool> _states;
        std::vector<const Interface*> _all_interfaces;
    };
}

#endif /* NETWORKPDA_H */

