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

#include <aalwines/model/CegarVerifier.h>
#include <aalwines/utils/json_stream.h>
#include <aalwines/utils/stopwatch.h>
#include <aalwines/utils/outcome.h>
#include <aalwines/query/QueryBuilder.h>
#include <aalwines/model/NetworkPDAFactory.h>
#include <aalwines/model/NetworkWeight.h>
#include <pdaaal/SolverAdapter.h>
#include <pdaaal/Reducer.h>

#include <boost/program_options.hpp>
namespace po = boost::program_options;

namespace aalwines {

    inline void to_json(json & j, const Query::mode_t& mode) {
        static const char *modeTypes[] {"OVER", "UNDER", "DUAL", "EXACT"};
        j = modeTypes[mode];
    }

    class Verifier {
    public:

        explicit Verifier(const std::string& caption = "Verification Options") : verification(caption) {
            verification.add_options()
                    ("engine,e", po::value<size_t>(&_engine), "0=no verification,1=post*,2=pre*,3=post*CEGAR,4=post*CEGARwithSimpleRefinement,5=post*NoAbstraction")
                    ("tos-reduction,r", po::value<size_t>(&_reduction), "0=none,1=simple,2=dual-stack,3=simple+backup,4=dual-stack+backup")
                    ("trace,t", po::bool_switch(&_print_trace), "Get a trace when possible")
                    ;
        }

        [[nodiscard]] const po::options_description& options() const { return verification; }
        auto add_options() { return verification.add_options(); }

        void check_settings() const {
            if(_reduction > 4) {
                std::cerr << "Unknown value for --tos-reduction : " << _reduction << std::endl;
                exit(-1);
            }
            if(_engine > 5) {
                std::cerr << "Unknown value for --engine : " << _engine << std::endl;
                exit(-1);
            }
        }
        void check_supports_weight() const {
            if (_engine != 1) {
                std::cerr << "Shortest trace using weights is only implemented for --engine 1 (post*). Not for --engine " << _engine << std::endl;
                exit(-1);
            }
        }
        void set_print_trace() { _print_trace = true; }

        template<typename W_FN = std::function<void(void)>>
        void run(Builder& builder, const std::vector<std::string>& query_strings, json_stream& json_output, bool print_timing = true, const W_FN& weight_fn = [](){}) {
            size_t query_no = 0;
            for (auto& q : builder._result) {
                std::stringstream qn;
                qn << "Q" << query_no+1;

                auto res = run_once(builder, q, print_timing, weight_fn);
                res["query"] = query_strings[query_no];
                json_output.entry_object(qn.str(), res);

                ++query_no;
            }
        }

        template<typename W_FN = std::function<void(void)>>
        json run_once(Builder& builder, Query& q, bool print_timing = true, const W_FN& weight_fn = [](){}){
            constexpr static bool is_weighted = pdaaal::is_weighted<typename W_FN::result_type>;

            json output; // Store output information in this JSON object.
            static const char *engineTypes[] {"", "Post*", "Pre*", "CEGAR_Post*", "CEGAR_Post*_SimpleRefinement", "CEGAR_NoAbstraction_Post*"};
            output["engine"] = engineTypes[_engine];

            // DUAL mode means first do OVER-approximation, then if that is inconclusive, do UNDER-approximation
            std::vector<Query::mode_t> modes = q.approximation() == Query::DUAL ? std::vector<Query::mode_t>{Query::OVER, Query::UNDER} : std::vector<Query::mode_t>{q.approximation()};
            output["mode"] = q.approximation();
            q.set_approximation(modes[0]);

            json json_trace;
            std::vector<unsigned int> trace_weight;
            stopwatch full_time(false);

            utils::outcome_t result = utils::outcome_t::MAYBE;
            if (_engine == 5) {
                assert(q.number_of_failures() == 0); // k>0 not yet supported for CEGAR.
                output["no_abstraction"] = json::object();
                full_time.start();
                auto res = CegarVerifier::verify<true>(builder._network, q, builder.all_labels(), output["no_abstraction"]);
                full_time.stop();
                if (res) {
                    result = utils::outcome_t::YES;
                    json_trace = res.value();
                } else {
                    result = utils::outcome_t::NO;
                }
            } else if (_engine == 4) {
                assert(q.number_of_failures() == 0); // k>0 not yet supported for CEGAR.
                output["abstraction"] = json::object();
                full_time.start();
                auto res = CegarVerifier::verify<false,pdaaal::refinement_option_t::fast_refinement>(builder._network, q, builder.all_labels(), output["abstraction"]);
                full_time.stop();
                if (res) {
                    result = utils::outcome_t::YES;
                    json_trace = res.value();
                } else {
                    result = utils::outcome_t::NO;
                }
            } else if (_engine == 3) {
                assert(q.number_of_failures() == 0); // k>0 not yet supported for CEGAR.
                output["abstraction"] = json::object();
                full_time.start();
                auto res = CegarVerifier::verify<false,pdaaal::refinement_option_t::best_refinement>(builder._network, q, builder.all_labels(), output["abstraction"]);
                full_time.stop();
                if (res) {
                    result = utils::outcome_t::YES;
                    json_trace = res.value();
                } else {
                    result = utils::outcome_t::NO;
                }
            } else {
                stopwatch compilation_time(false);
                stopwatch reduction_time(false);
                stopwatch reachability_time(false);
                stopwatch trace_making_time(false);
                full_time.start();
                for (auto m : modes) {
                    json_trace = json(); // Clear trace from previous mode.

                    // Construct PDA
                    compilation_time.start();
                    q.set_approximation(m);
                    q.compile_nfas();
                    NetworkPDAFactory factory(q, builder._network, builder.all_labels(), weight_fn);
                    auto problem_instance = factory.compile(q.construction(), q.destruction());
                    compilation_time.stop();

                    // Reduce PDA
                    reduction_time.start(); // TODO: Implement reduction for solver instance. (Generalised version with multiple initial and final states)
                    //output["reduction"] = pdaaal::Reducer::reduce(pda, _reduction, pda.initial(), pda.terminal());
                    reduction_time.stop();

                    // Choose engine, run verification, and (if relevant) get the trace.
                    reachability_time.start();
                    bool engine_outcome;
                    switch (_engine) {
                        case 1: {
                            constexpr pdaaal::Trace_Type trace_type = is_weighted ? pdaaal::Trace_Type::Shortest
                                                                                  : pdaaal::Trace_Type::Any;
                            engine_outcome = pdaaal::Solver::post_star_accepts<trace_type>(problem_instance);
                            reachability_time.stop();
                            trace_making_time.start();
                            if (engine_outcome) {
                                std::vector<pdaaal::TypedPDA<Query::label_t>::tracestate_t> pda_trace;
                                if constexpr (is_weighted) {
                                    std::tie(pda_trace, trace_weight) = pdaaal::Solver::get_trace<trace_type>(
                                            problem_instance);
                                } else {
                                    pda_trace = pdaaal::Solver::get_trace<trace_type>(problem_instance);
                                }
                                json_trace = factory.get_json_trace(pda_trace);
                                if (!json_trace.is_null()) result = utils::outcome_t::YES;
                            }
                            trace_making_time.stop();
                            break;
                        }
                        case 2: {
                            engine_outcome = pdaaal::Solver::pre_star_accepts(problem_instance);
                            reachability_time.stop();
                            trace_making_time.start();
                            if (engine_outcome) {
                                auto pda_trace = pdaaal::Solver::get_trace(problem_instance);
                                json_trace = factory.get_json_trace(pda_trace);
                                if (!json_trace.is_null()) result = utils::outcome_t::YES;
                            }
                            trace_making_time.stop();
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
                full_time.stop();
                if (print_timing) {
                    output["compilation-time"] = compilation_time.duration();
                    output["reduction-time"] = reduction_time.duration();
                    output["reachability-time"] = reachability_time.duration();
                    output["trace-making-time"] = trace_making_time.duration();
                }
            }

            output["result"] = result;

            if (_print_trace && result == utils::outcome_t::YES) {
                if constexpr (is_weighted) {
                    output["trace-weight"] = trace_weight;
                }
                output["trace"] = json_trace;
            }
            if (print_timing) {
                output["full-time"] = full_time.duration();
            }

            return output;
        }

    private:
        po::options_description verification;

        // Settings
        size_t _engine = 1;
        size_t _reduction = 0;
        bool _print_trace = false;
    };

}

#endif //AALWINES_VERIFIER_H
