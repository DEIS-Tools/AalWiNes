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

#include <boost/program_options.hpp>
namespace po = boost::program_options;

namespace pdaaal {
    std::istream& operator>>(std::istream& in, Trace_Type& trace_type) {
        std::string token;
        in >> token;
        if (token == "0") {
            trace_type = Trace_Type::None;
        } else if (token == "1") {
            trace_type = Trace_Type::Any;
        } else if (token == "2") {
            trace_type = Trace_Type::Shortest;
        } else if (token == "3") {
            trace_type = Trace_Type::Longest;
        } else {
            in.setstate(std::ios_base::failbit);
        }
        return in;
    }
    constexpr std::ostream& operator<<(std::ostream& s, const Trace_Type& trace_type) {
        switch (trace_type) {
            case Trace_Type::None:
                s << "0";
                break;
            case Trace_Type::Any:
                s << "1";
                break;
            case Trace_Type::Shortest:
                s << "2";
                break;
            case Trace_Type::Longest:
                s << "3";
                break;
            case Trace_Type::ShortestFixedPoint:
                s << "4";
                break;
        }
        return s;
    }
}

namespace aalwines {

    inline void to_json(json & j, const Query::mode_t& mode) {
        switch (mode) {
            case Query::mode_t::OVER:
                j = "OVER";
                break;
            case Query::mode_t::EXACT:
                j = "EXACT";
                break;
        }
    }

    class Verifier {
    public:

        explicit Verifier(const std::string& caption = "Verification Options") : verification(caption) {
            verification.add_options()
                    ("engine,e", po::value<size_t>(&_engine), "0=no verification,1=post*,2=pre*,3=post*CEGAR,4=post*CEGARwithSimpleRefinement,5=post*NoAbstraction,6=dual*,9=dual*CEGAR")
                    ("trace,t", po::value<pdaaal::Trace_Type>(&_trace_type)->default_value(pdaaal::Trace_Type::None), "Trace type. 0=no trace, 1=any trace, 2=shortest trace, 3=longest trace")
                    ;
        }

        [[nodiscard]] const po::options_description& options() const { return verification; }
        auto add_options() { return verification.add_options(); }

        void check_settings() const {
            if(_engine > 9) {
                std::cerr << "Unknown value for --engine : " << _engine << std::endl;
                exit(-1);
            }
        }
        void check_supports_weight() const {
            if (!((_engine == 1 && _trace_type == pdaaal::Trace_Type::Shortest) ||
                  (_engine == 2 && _trace_type == pdaaal::Trace_Type::Longest))) {
                std::cerr << "Shortest trace using weights is only implemented for --engine 1 (post*), or longest trace (--trace 3) for --engine 2 (pre*). Not for --engine " << _engine << std::endl;
                exit(-1);
            }
        }
        void set_trace_type(pdaaal::Trace_Type trace_type) { _trace_type = trace_type; }

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
            using weight_type = pdaaal::weight<typename W_FN::result_type>;
            constexpr static bool is_weighted = pdaaal::is_weighted<weight_type>;

            json output; // Store output information in this JSON object.
            static const char *engineTypes[] {"", "Post*", "Pre*", "CEGAR_Post*", "CEGAR_Post*_SimpleRefinement", "CEGAR_NoAbstraction_Post*", "DualSearch", "Post* (no ET)", "Pre* (no ET)", "CEGAR_Dual"};
            output["engine"] = engineTypes[_engine];

            // DUAL mode means first do OVER-approximation, then if that is inconclusive, do UNDER-approximation
            std::vector<Query::mode_t> modes = std::vector<Query::mode_t>{q.approximation()};
            output["mode"] = q.approximation();
            q.set_approximation(modes[0]);

            json json_trace;
            std::vector<unsigned int> trace_weight;
            stopwatch full_time(false);

            utils::outcome_t result = utils::outcome_t::MAYBE;
            if (_engine == 5) {
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
                output["abstraction"] = json::object();
                full_time.start();
                auto res = CegarVerifier::verify<false,false,pdaaal::refinement_option_t::fast_refinement>(builder._network, q, builder.all_labels(), output["abstraction"]);
                full_time.stop();
                if (res) {
                    result = utils::outcome_t::YES;
                    json_trace = res.value();
                } else {
                    result = utils::outcome_t::NO;
                }
            } else if (_engine == 3) {
                output["abstraction"] = json::object();
                full_time.start();
                auto res = CegarVerifier::verify<false,false,pdaaal::refinement_option_t::best_refinement>(builder._network, q, builder.all_labels(), output["abstraction"]);
                full_time.stop();
                if (res) {
                    result = utils::outcome_t::YES;
                    json_trace = res.value();
                } else {
                    result = utils::outcome_t::NO;
                }
            } else if (_engine == 9) {
                output["abstraction"] = json::object();
                full_time.start();
                auto res = CegarVerifier::verify<false,false,pdaaal::refinement_option_t::best_refinement,true>(builder._network, q, builder.all_labels(), output["abstraction"]);
                full_time.stop();
                if (res) {
                    result = utils::outcome_t::YES;
                    json_trace = res.value();
                } else {
                    result = utils::outcome_t::NO;
                }
            } else {
                stopwatch compilation_time(false);
                stopwatch reachability_time(false);
                stopwatch trace_making_time(false);
                full_time.start();
                for (auto m : modes) {
                    json_trace = json(); // Clear trace from previous mode.

                    // Construct PDA
                    compilation_time.start();
                    q.set_approximation(m);
                    q.compile_nfas();

                    bool engine_outcome;
                    switch (_trace_type) {
                        case pdaaal::Trace_Type::None:
                        case pdaaal::Trace_Type::Any:
                            engine_outcome = run_once_impl<pdaaal::Trace_Type::Any>(builder, q, weight_fn, trace_weight, result, json_trace, compilation_time, reachability_time, trace_making_time);
                            break;
                        case pdaaal::Trace_Type::Shortest:
                            engine_outcome = run_once_impl<pdaaal::Trace_Type::Shortest>(builder, q, weight_fn, trace_weight, result, json_trace, compilation_time, reachability_time, trace_making_time);
                            break;
                        case pdaaal::Trace_Type::Longest:
                            engine_outcome = run_once_impl<pdaaal::Trace_Type::Longest>(builder, q, weight_fn, trace_weight, result, json_trace, compilation_time, reachability_time, trace_making_time);
                            break;
                        case pdaaal::Trace_Type::ShortestFixedPoint:
                            engine_outcome = run_once_impl<pdaaal::Trace_Type::ShortestFixedPoint>(builder, q, weight_fn, trace_weight, result, json_trace, compilation_time, reachability_time, trace_making_time);
                            break;
                    }

                    // Determine result from the outcome of verification and the mode (over/under-approximation) used.
                    if (q.number_of_failures() == 0) {
                        result = engine_outcome ? utils::outcome_t::YES : utils::outcome_t::NO;
                    }
                    if (result == utils::outcome_t::MAYBE && m == Query::mode_t::OVER && !engine_outcome) {
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
                    output["reachability-time"] = reachability_time.duration();
                    output["trace-making-time"] = trace_making_time.duration();
                }
            }

            output["result"] = result;

            if (_trace_type != pdaaal::Trace_Type::None && result == utils::outcome_t::YES) {
                if constexpr (is_weighted) {
                    if (trace_weight == pdaaal::max_weight<typename weight_type::type>::bottom()) {
                        output["trace-weight"] = "infinite";
                    } else {
                        output["trace-weight"] = trace_weight;
                    }
                }
                output["trace"] = json_trace;
            }
            if (print_timing) {
                output["full-time"] = full_time.duration();
            }

            return output;
        }

    private:
        template<pdaaal::Trace_Type trace_type, typename W_FN>
        bool run_once_impl(Builder& builder, Query& q, const W_FN& weight_fn,
                           std::vector<unsigned int>& trace_weight, utils::outcome_t& result, json& json_trace,
                           stopwatch& compilation_time, stopwatch& reachability_time, stopwatch& trace_making_time){
            using weight_type = pdaaal::weight<typename W_FN::result_type>;
            constexpr static bool is_weighted = pdaaal::is_weighted<weight_type>;

            bool engine_outcome;
            if constexpr (trace_type == pdaaal::Trace_Type::Longest) {
                if constexpr(!is_weighted) {
                    throw base_error("error: Longest trace option requires weight to be specified.");
                } else {
                    compilation_time.start();
                    auto factory = makeNetworkPDAFactory<pdaaal::TraceInfoType::Pair>(q, builder._network, builder.all_labels(), weight_fn);
                    auto problem_instance = factory.compile(q.construction(), q.destruction());
                    compilation_time.stop();
                    if (_engine == 2) {
                        engine_outcome = pdaaal::Solver::pre_star_fixed_point_accepts<trace_type>(*problem_instance);
                        reachability_time.stop();
                        trace_making_time.start();
                        if (engine_outcome) {
                            std::vector<typename decltype(factory)::trace_state_t> pda_trace;
                            std::tie(pda_trace, trace_weight) = pdaaal::Solver::get_trace<trace_type>(*problem_instance);
                            if (trace_weight == pdaaal::max_weight<typename weight_type::type>::bottom()) {
                                result = utils::outcome_t::YES; // TODO: This might not be correct. We need a trace to validate!!
                            } else {
                                json_trace = factory.get_json_trace(pda_trace);
                                if (!json_trace.is_null()) result = utils::outcome_t::YES;
                            }
                        }
                        trace_making_time.stop();
                    } else {
                        throw base_error("Longest trace option only supported for pre* (--engine 2).");
                    }
                }
            } else if constexpr (trace_type == pdaaal::Trace_Type::ShortestFixedPoint) {
                throw base_error("Shortest trace using fixed point computation not yet supported."); // We could, but it is not a possible option, so this should not happen.
            } else {
                compilation_time.start();
                NetworkPDAFactory factory(q, builder._network, builder.all_labels(), weight_fn);
                auto problem_instance = factory.compile(q.construction(), q.destruction());
                compilation_time.stop();
                if constexpr (trace_type == pdaaal::Trace_Type::Shortest) {
                    if constexpr(!is_weighted) {
                        throw base_error("error: Shortest trace option requires weight to be specified.");
                    } else {
                        if (_engine == 1) {
                            engine_outcome = pdaaal::Solver::post_star_accepts<trace_type>(*problem_instance);
                            reachability_time.stop();
                            trace_making_time.start();
                            if (engine_outcome) {
                                std::vector<typename decltype(factory)::trace_state_t> pda_trace;
                                std::tie(pda_trace, trace_weight) = pdaaal::Solver::get_trace<trace_type>(*problem_instance);
                                json_trace = factory.get_json_trace(pda_trace);
                                if (!json_trace.is_null()) result = utils::outcome_t::YES;
                            }
                            trace_making_time.stop();
                        } else {
                            throw base_error("Shortest trace option only supported for post* (--engine 1).");
                        }
                    }
                } else {
                    switch (_engine) {
                        case 1:
                            engine_outcome = pdaaal::Solver::post_star_accepts<pdaaal::Trace_Type::Any>(*problem_instance);
                            break;
                        case 2:
                            engine_outcome = pdaaal::Solver::pre_star_accepts(*problem_instance);
                            break;
                        case 6:
                            engine_outcome = pdaaal::Solver::dual_search_accepts(*problem_instance);
                            break;
                        case 7:
                            engine_outcome = pdaaal::Solver::post_star_accepts_no_ET(*problem_instance);
                            break;
                        case 8:
                            engine_outcome = pdaaal::Solver::pre_star_accepts_no_ET(*problem_instance);
                            break;
                        default:
                            throw base_error("Unsupported --engine value given");
                    }
                    reachability_time.stop();
                    trace_making_time.start();
                    if (engine_outcome) {
                        auto pda_trace = pdaaal::Solver::get_trace(*problem_instance);
                        json_trace = factory.get_json_trace(pda_trace);
                        if (!json_trace.is_null()) result = utils::outcome_t::YES;
                    }
                    trace_making_time.stop();
                }
            }
            return engine_outcome;
        }

    private:
        po::options_description verification;

        // Settings
        size_t _engine = 1;
        // size_t _reduction = 0;
        // bool _print_trace = false;
        pdaaal::Trace_Type _trace_type = pdaaal::Trace_Type::None;
    };

}

#endif //AALWINES_VERIFIER_H
