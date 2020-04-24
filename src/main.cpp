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

//
// Created by Peter G. Jensen
//

#include <aalwines/utils/errors.h>
#include <aalwines/query/QueryBuilder.h>

#include <aalwines/model/builders/JuniperBuilder.h>
#include <aalwines/model/builders/PRexBuilder.h>

#include <aalwines/model/NetworkPDAFactory.h>
#include <aalwines/model/NetworkWeight.h>

#include <aalwines/query/parsererrors.h>
#include <pdaaal/PDAFactory.h>
#include <aalwines/engine/Moped.h>
#include <pdaaal/SolverAdapter.h>
#include <pdaaal/Reducer.h>
#include <aalwines/utils/stopwatch.h>
#include <aalwines/utils/outcome.h>

#include <boost/program_options.hpp>

#include <iostream>
#include <string>
#include <vector>
#include <fstream>

namespace po = boost::program_options;
using namespace aalwines;
using namespace pdaaal;

template<typename W_FN>
bool do_verification(stopwatch& compilation_time, stopwatch& reduction_time, stopwatch& verification_time,
        Query& q, Query::mode_t m, Network& network, bool no_ip_swap, std::pair<size_t,size_t>& reduction, size_t tos,
        bool need_trace, size_t engine, Moped& moped, SolverAdapter& solver, utils::outcome_t& result,
        std::vector<pdaaal::TypedPDA<Query::label_t>::tracestate_t >& trace, std::stringstream& proof,
        const W_FN& weight_fn) {
    compilation_time.start();
    q.set_approximation(m);
    NetworkPDAFactory factory(q, network, no_ip_swap, weight_fn);
    auto pda = factory.compile();
    compilation_time.stop();

    reduction_time.start();
    reduction = Reducer::reduce(pda, tos, pda.initial(), pda.terminal());
    reduction_time.stop();

    verification_time.start();
    bool engine_outcome;
    switch(engine) {
        case 1:
            engine_outcome = moped.verify(pda, need_trace);
            verification_time.stop();
            if(need_trace && engine_outcome) {
                trace = moped.get_trace(pda);
                if (factory.write_json_trace(proof, trace))
                    result = utils::YES;
            }
            break;
        case 2: {
            using W = typename W_FN::result_type;
            SolverAdapter::res_type<W,std::less<W>,pdaaal::add<W>> solver_result;
            if constexpr (pdaaal::is_weighted<typename W_FN::result_type>) {
                solver_result = solver.post_star<pdaaal::Trace_Type::Shortest>(pda);
            } else {
                solver_result = solver.post_star<pdaaal::Trace_Type::Any>(pda);
            }
            engine_outcome = solver_result.first;
            verification_time.stop();
            if (need_trace && engine_outcome) {
                trace = solver.get_trace(pda, std::move(solver_result.second));
                if (factory.write_json_trace(proof, trace))
                    result = utils::YES;
            }
            break;
        }
        case 3: {
            auto solver_result = solver.pre_star(pda, need_trace);
            engine_outcome = solver_result.first;
            verification_time.stop();
            if (need_trace && engine_outcome) {
                trace = solver.get_trace(pda, std::move(solver_result.second));
                if (factory.write_json_trace(proof, trace))
                    result = utils::YES;
            }
            break;
        }
        default:
            throw illegal_option_error("Unsupported value", "--engine", std::to_string(engine));
    }
    return engine_outcome;
}


/*
 TODO:
 * fix error-handling
 * improve on throws/exception-types
 * fix slots
 */
int main(int argc, const char** argv)
{
    std::stringstream  outstream;
    try {
        po::options_description opts;
        opts.add_options()
                ("help,h", "produce help message");

        po::options_description output("Output Options");
        po::options_description input("Input Options");
        po::options_description verification("Verification Options");

        bool print_dot = false;
        bool print_net = false;
        bool no_parser_warnings = false;
        bool silent = false;
        bool dump_to_moped = false;
        bool no_timing = false;
        std::string topology_destination;
        std::string routing_destination;

        output.add_options()
                ("dot", po::bool_switch(&print_dot), "A dot output will be printed to cout when set.")
                ("net", po::bool_switch(&print_net), "A json output of the network will be printed to cout when set.")
                ("disable-parser-warnings,W", po::bool_switch(&no_parser_warnings), "Disable warnings from parser.")
                ("silent,s", po::bool_switch(&silent), "Disables non-essential output (implies -W).")
                ("no-timing", po::bool_switch(&no_timing), "Disables timing output")
                ("dump-for-moped", po::bool_switch(&dump_to_moped),
                 "Dump the constructed PDA in a MOPED format (expects a singleton query-file).")
                ("write-topology", po::value<std::string>(&topology_destination),
                 "Write the topology in the P-Rex format to the given file.")
                ("write-routing", po::value<std::string>(&routing_destination),
                 "Write the Routing in the P-Rex format to the given file.");


        std::string junos_config, prex_topo, prex_routing;
        bool skip_pfe = false;
        input.add_options()
                ("juniper", po::value<std::string>(&junos_config),
                 "A file containing a network-description; each line is a router in the format \"name,alias1,alias2:adjacency.xml,mpls.xml,pfe.xml\". ")
                ("topology", po::value<std::string>(&prex_topo),
                 "An xml-file defining the topology in the P-Rex format")
                ("routing", po::value<std::string>(&prex_routing),
                 "An xml-file defining the routing in the P-Rex format")
                ("skip-pfe", po::bool_switch(&skip_pfe),
                 "Skip \"indirect\" cases of juniper-routing as package-drops (compatability with P-Rex semantics).");

        std::string query_file;
        std::string weight_file;
        unsigned int link_failures = 0;
        size_t tos = 0;
        size_t engine = 0;
        bool get_trace = false;
        bool no_ip_swap = false;
        verification.add_options()
                ("query,q", po::value<std::string>(&query_file),
                 "A file containing valid queries over the input network.")
                ("trace,t", po::bool_switch(&get_trace), "Get a trace when possible")
                ("no-ip-route", po::bool_switch(&no_ip_swap), "Disable encoding of routing via IP")
                ("link,l", po::value<unsigned int>(&link_failures), "Number of link-failures to model.")
                ("tos-reduction,r", po::value<size_t>(&tos), "0=none,1=simple,2=dual-stack,3=dual-stack+backup")
                ("engine,e", po::value<size_t>(&engine), "0=no verification,1=moped,2=post*,3=pre*")
                ("weight,w", po::value<std::string>(&weight_file), "A file containing the weight function expression");

        opts.add(input);
        opts.add(output);
        opts.add(verification);

        po::variables_map vm;
        po::store(po::parse_command_line(argc, argv, opts), vm);
        po::notify(vm);

        if (vm.count("help")) {
            std::cout << opts << "\n";
            return 1;
        }

        if (tos > 3)
            throw illegal_option_error("Unknown value for", "--tos-reduction", std::to_string(tos));

        if (engine > 3)
            throw illegal_option_error("Unknown value for", "--engine", std::to_string(engine));

        if (engine != 0 && dump_to_moped)
            throw base_error("Cannot both verify (--engine > 0) and dump model (--dump-for-moped) to stdout");

        if (!junos_config.empty() && (!prex_routing.empty() || !prex_topo.empty()))
            throw base_error("--junos cannot be used with --topology or --routing.");

        if (prex_routing.empty() != prex_topo.empty())
            throw base_error("Both --topology and --routing have to be non-empty (or both have to be empty).");

        if (junos_config.empty() && prex_routing.empty() && prex_topo.empty())
            throw base_error("Either a Junos configuration or a P-Rex configuration must be given.");

        if (skip_pfe && junos_config.empty())
            throw base_error("--skip-pfe is only avaliable for --junos configurations.");

        if (silent) no_parser_warnings = true;

        std::stringstream dummy;
        std::ostream &warnings = no_parser_warnings ? dummy : std::cerr;

        stopwatch parsingwatch;
        auto network = junos_config.empty() ?
                       PRexBuilder::parse(prex_topo, prex_routing, warnings) :
                       JuniperBuilder::parse(junos_config, warnings, skip_pfe);
        parsingwatch.stop();

        if (print_dot) {
            network.print_dot(outstream);
        }

        if (!topology_destination.empty()) {
            std::ofstream out(topology_destination);
            if (out.is_open())
                network.write_prex_topology(out);
            else throw file_not_found_error("Could not open --write-topology", topology_destination);
        }
        if (!routing_destination.empty()) {
            std::ofstream out(routing_destination);
            if (out.is_open())
                network.write_prex_routing(out);
            else throw file_not_found_error("Could not open --write-routing",  topology_destination);

        }

        if (!dump_to_moped && (print_net || !query_file.empty())) {
            outstream << "{\n";
            if (print_net) {
                network.print_json(outstream);
                if (!query_file.empty()) {
                    outstream << ",\n";
                }
            }
        }

        if (!query_file.empty()) {
            stopwatch queryparsingwatch;
            Builder builder(network);
            {
                std::ifstream qstream(query_file);
                if (!qstream.is_open())
                    throw file_not_found_error("Could not open --query", query_file);

                builder.do_parse(qstream);
                qstream.close();
            }
            queryparsingwatch.stop();

            std::optional<NetworkWeight::weight_function> weight_fn;
            if (!weight_file.empty()) {
                if (engine != 2)
                    throw base_error("Shortest trace using weights is only implemented for --engine 2 (post*). Not for --engine " + std::to_string(engine));
                // TODO: Implement parsing of latency info here.
                NetworkWeight network_weight;
                {
                    std::ifstream wstream(weight_file);
                    if (!wstream.is_open())
                        throw file_not_found_error("Could not open --weight",  weight_file);
                    weight_fn.emplace(network_weight.parse(wstream));
                    wstream.close();
                }
            } else {
                weight_fn = std::nullopt;
            }

            if (!dump_to_moped) {
                if (!no_timing) {
                    outstream << "\t\"network-parsing-time\":" << (parsingwatch.duration())
                              << ", \"query-parsing-time\":" << (queryparsingwatch.duration()) << ",\n";
                }
                outstream << "\t\"answers\":{\n";
            }

            Moped moped;
            SolverAdapter solver;
            size_t query_no = 0;
            for (auto &q : builder._result) {
                ++query_no;
                stopwatch compilation_time(false);
                if (dump_to_moped) {
                    compilation_time.start();
                    NetworkPDAFactory factory(q, network, no_ip_swap);
                    auto pda = factory.compile();
                    Moped::dump_pda(pda, outstream);
                } else {
                    std::vector<Query::mode_t> modes{q.approximation()};
                    bool was_dual = q.approximation() == Query::DUAL;
                    if (was_dual)
                        modes = std::vector<Query::mode_t>{Query::OVER, Query::UNDER};
                    std::pair<size_t, size_t> reduction;
                    utils::outcome_t result = utils::MAYBE;
                    stopwatch reduction_time(false);
                    stopwatch verification_time(false);
                    std::vector<pdaaal::TypedPDA<Query::label_t>::tracestate_t> trace;
                    std::stringstream proof;
                    bool need_trace = was_dual || get_trace;
                    for (auto m : modes) {
                        bool engine_outcome;
                        if (weight_fn) {
                            engine_outcome = do_verification(compilation_time, reduction_time,
                                                             verification_time, q, m, network, no_ip_swap, reduction,
                                                             tos, need_trace, engine,
                                                             moped, solver, result, trace, proof, weight_fn.value());
                        } else {
                            engine_outcome = do_verification<std::function<void(void)>>(compilation_time,
                                                                                        reduction_time,
                                                                                        verification_time, q, m,
                                                                                        network, no_ip_swap, reduction,
                                                                                        tos, need_trace, engine,
                                                                                        moped, solver, result, trace,
                                                                                        proof, []() {});
                        }
                        if (q.number_of_failures() == 0)
                            result = engine_outcome ? utils::YES : utils::NO;

                        if (result == utils::MAYBE && m == Query::OVER && !engine_outcome)
                            result = utils::NO;
                        if (result != utils::MAYBE)
                            break;
                        /*else
                            trace.clear();*/
                    }

                    // move this into function that generalizes
                    // and extracts trace at the same time.

                    outstream << "\t\"Q" << query_no << "\" : {\n\t\t\"result\":";
                    switch (result) {
                        case utils::MAYBE:
                            outstream << "null";
                            break;
                        case utils::NO:
                            outstream << "false";
                            break;
                        case utils::YES:
                            outstream << "true";
                            break;
                    }
                    outstream << ",\n";
                    outstream << "\t\t\"reduction\":[" << reduction.first << ", " << reduction.second << "]";
                    if (get_trace && result == utils::YES) {
                        outstream << ",\n\t\t\"trace\":[\n";
                        outstream << proof.str();
                        outstream << "\n\t\t]";
                    }
                    if (!no_timing) {
                        outstream << ",\n\t\t\"compilation-time\":" << (compilation_time.duration())
                                  << ",\n\t\t\"reduction-time\":" << (reduction_time.duration())
                                  << ",\n\t\t\"verification-time\":" << (verification_time.duration());
                    }
                    outstream << "\n\t}";
                    if (query_no != builder._result.size())
                        outstream << ",";
                    outstream << "\n";
                }
            }

            if (!dump_to_moped) {
                outstream << "\n}";
            }
        }
        if (!dump_to_moped && (print_net || !query_file.empty())) {
            outstream << "\n}\n";
        }
    }
    catch(base_error& error)
    {
        std::cout << "{\"error\":\n";
        error.print_json(std::cout);
        std::cout << "\n}" << std::endl;
        return -1;
    }
    std::cout << outstream.str() << std::endl;
    return 0;
}
