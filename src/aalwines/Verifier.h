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
 * File:   Verifier.h
 * Author: Morten K. Schou <morten@h-schou.dk>
 *
 * Created on 13-08-2020.
 */

#ifndef AALWINES_VERIFIER_H
#define AALWINES_VERIFIER_H

#include <aalwines/utils/json_stream.h>
#include <aalwines/utils/stopwatch.h>
#include <aalwines/utils/outcome.h>
#include <aalwines/query/QueryBuilder.h>
#include <aalwines/model/NetworkPDAFactory.h>
#include <aalwines/model/NetworkWeight.h>
#include <pdaaal/SolverAdapter.h>
#include <pdaaal/Reducer.h>

namespace aalwines {

    inline void to_json(json & j, const Query::mode_t& mode) {
        static const char *modeTypes[] {"OVER", "UNDER", "DUAL", "EXACT"};
        j = modeTypes[mode];
    }

    using namespace pdaaal;

    template<typename W_FN = std::function<void(void)>>
    class Verifier {
    public:
        constexpr static bool is_weighted = pdaaal::is_weighted<typename W_FN::result_type>;

        explicit Verifier(Builder& builder, size_t engine = 1, size_t reduction = 0, bool no_ip_swap = false, bool print_timing = true, bool print_trace = true)
        : Verifier(builder, [](){}, engine, reduction, no_ip_swap, print_timing, print_trace) {};
        Verifier(Builder& builder, const W_FN& weight_fn, size_t engine = 1, size_t reduction = 0, bool no_ip_swap = false, bool print_timing = true, bool print_trace = true)
        : _builder(builder), weight_fn(weight_fn), engine(engine), reduction(reduction), no_ip_swap(no_ip_swap), print_timing(print_timing), print_trace(print_trace) {};

        void run(const std::vector<std::string>& query_strings, json_stream& json_output) {
            size_t query_no = 0;
            for (auto& q : _builder._result) {
                std::stringstream qn;
                qn << "Q" << query_no+1;

                auto res = run_once(q);
                res["query"] = query_strings[query_no];
                json_output.entry_object(qn.str(), res);

                ++query_no;
            }
        }

        json run_once(Query& q){
            json output; // Store output information in this JSON object.
            static const char *engineTypes[] {"", "Post*", "Pre*"};
            output["engine"] = engineTypes[engine];

            // DUAL mode means first do OVER-approximation, then if that is inconclusive, do UNDER-approximation
            std::vector<Query::mode_t> modes = q.approximation() == Query::DUAL ? std::vector<Query::mode_t>{Query::OVER, Query::UNDER} : std::vector<Query::mode_t>{q.approximation()};
            output["mode"] = q.approximation();

            std::stringstream proof;
            std::vector<unsigned int> trace_weight;
            stopwatch compilation_time(false);
            stopwatch reduction_time(false);
            stopwatch verification_time(false);

            utils::outcome_t result = utils::outcome_t::MAYBE;
            for (auto m : modes) {
                proof = std::stringstream(); // Clear stream from previous mode.

                // Construct PDA
                compilation_time.start();
                q.set_approximation(m);
                NetworkPDAFactory factory(q, _builder._network, _builder.all_labels(), no_ip_swap, weight_fn);
                auto pda = factory.compile();
                compilation_time.stop();

                // Reduce PDA
                reduction_time.start();
                output["reduction"] = Reducer::reduce(pda, reduction, pda.initial(), pda.terminal());
                reduction_time.stop();

                // Choose engine, run verification, and (if relevant) get the trace.
                verification_time.start();
                bool engine_outcome;
                switch(engine) {
                    case 1: {
                        using W = typename W_FN::result_type;
                        SolverAdapter::res_type<W,std::less<W>,pdaaal::add<W>> solver_result;
                        if constexpr (is_weighted) {
                            solver_result = solver.post_star<pdaaal::Trace_Type::Shortest>(pda);
                        } else {
                            solver_result = solver.post_star<pdaaal::Trace_Type::Any>(pda);
                        }
                        engine_outcome = solver_result.first;
                        verification_time.stop();
                        if (engine_outcome) {
                            std::vector<pdaaal::TypedPDA<Query::label_t>::tracestate_t > trace;
                            if constexpr (is_weighted) {
                                std::tie(trace, trace_weight) = solver.get_trace<pdaaal::Trace_Type::Shortest>(pda, std::move(solver_result.second));
                            } else {
                                trace = solver.get_trace<pdaaal::Trace_Type::Any>(pda, std::move(solver_result.second));
                            }
                            if (factory.write_json_trace(proof, trace))
                                result = utils::outcome_t::YES;
                        }
                        break;
                    }
                    case 2: {
                        auto solver_result = solver.pre_star(pda, true);
                        engine_outcome = solver_result.first;
                        verification_time.stop();
                        if (engine_outcome) {
                            auto trace = solver.get_trace(pda, std::move(solver_result.second));
                            if (factory.write_json_trace(proof, trace))
                                result = utils::outcome_t::YES;
                        }
                        break;
                    }
                    default:
                        throw base_error("Unsupported --engine value given");
                }

                // Determine result from the outcome of verification and the mode (over/under-approximation) used.
                if (q.number_of_failures() == 0) {
                    result = engine_outcome ? utils::outcome_t::YES : utils::outcome_t::NO;
                }
                if (result == utils::outcome_t::MAYBE && m == Query::OVER && !engine_outcome) {
                    result = utils::outcome_t::NO;
                }
                if (result != utils::outcome_t::MAYBE) {
                    output["mode"] = m;
                    break;
                }
            }

            output["result"] = result;

            if (print_trace && result == utils::outcome_t::YES) {
                if constexpr (is_weighted) {
                    output["trace-weight"] = trace_weight;
                }
                std::stringstream trace;
                trace << "[" << proof.str() << "]"; // TODO: Make NetworkPDAFactory::write_json_trace return a json object instead of ad-hoc formatting to a stringstream.
                output["trace"] = json::parse(trace.str());
            }
            if (print_timing) {
                output["compilation-time"] = compilation_time.duration();
                output["reduction-time"] = reduction_time.duration();
                output["verification-time"] = verification_time.duration();
            }

            return output;
        }

    private:
        // Builder contains the network and the queries.
        Builder& _builder;

        // Weight function
        const W_FN& weight_fn;

        // Settings
        size_t engine;
        size_t reduction;
        bool no_ip_swap;
        bool print_timing;
        bool print_trace;

        // Solver engines
        SolverAdapter solver;
    };

}

#endif //AALWINES_VERIFIER_H
