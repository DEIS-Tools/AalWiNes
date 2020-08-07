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
 * File:   Query_test.cpp
 * Author: Morten K. Schou <morten@h-schou.dk>
 *
 * Created on 10-04-2020
 */

#define BOOST_TEST_MODULE QueryTest

#include <boost/test/unit_test.hpp>
#include <aalwines/model/Network.h>
#include <aalwines/synthesis/RouteConstruction.h>
#include <aalwines/query/QueryBuilder.h>
#include <aalwines/utils/outcome.h>
#include <aalwines/model/NetworkPDAFactory.h>
#include <pdaaal/SolverAdapter.h>
#include <pdaaal/Reducer.h>

using namespace aalwines;

BOOST_AUTO_TEST_CASE(QueryTest1) {
    std::vector<std::string> routers{"Router0", "Router1"};
    std::vector<std::vector<std::string>> links{{"Router1"},{"Router0"}};

    auto network = Network::make_network(routers, links);
    uint64_t i = 42;
    auto next_label = [&i](){return Query::label_t(Query::type_t::MPLS, 0, i++);};
    RouteConstruction::make_data_flow(network.get_router(0)->find_interface("iRouter0"), network.get_router(1)->find_interface("iRouter1"), next_label);

    Builder builder(network);
    std::string query("<.> [.#Router0] [Router0#Router1] [Router1#.] <.> 0 OVER");


    // TODO: This stuff from main.cpp should be in its own file...
    std::istringstream qstream(query);
    builder.do_parse(qstream);
    size_t query_no = 0;
    pdaaal::SolverAdapter solver;
    for(auto& q : builder._result) {
        query_no++;
        std::vector<Query::mode_t> modes{q.approximation()};

        bool was_dual = q.approximation() == Query::DUAL;
        if (was_dual)
            modes = std::vector<Query::mode_t>{Query::OVER, Query::UNDER};
        std::pair<size_t, size_t> reduction;
        utils::outcome_t result = utils::MAYBE;
        std::vector<pdaaal::TypedPDA<Query::label_t>::tracestate_t> trace;
        std::stringstream proof;

        size_t tos = 0;
        bool no_ip_swap = false;
        bool get_trace = true;

        for (auto m : modes) {
            q.set_approximation(m);
            NetworkPDAFactory factory(q, network, builder.all_labels(), no_ip_swap);
            auto pda = factory.compile();
            reduction = pdaaal::Reducer::reduce(pda, tos, pda.initial(), pda.terminal());
            bool need_trace = was_dual || get_trace;

            auto solver_result1 = solver.post_star<pdaaal::Trace_Type::Any>(pda);
            bool engine_outcome = solver_result1.first;
            if (need_trace && engine_outcome) {
                trace = solver.get_trace(pda, std::move(solver_result1.second));
                if (factory.write_json_trace(proof, trace))
                    result = utils::YES;
            }
            if (q.number_of_failures() == 0)
                result = engine_outcome ? utils::YES : utils::NO;

            if (result == utils::MAYBE && m == Query::OVER && !engine_outcome)
                result = utils::NO;
            if (result != utils::MAYBE)
                break;
        }
        BOOST_CHECK_EQUAL(result, utils::YES);
    }
}

