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
#include <aalwines/utils/json_stream.h>
#include <aalwines/query/QueryBuilder.h>

#include <aalwines/model/builders/AalWiNesBuilder.h>
#include <aalwines/model/builders/TopologyBuilder.h>

#include <aalwines/model/NetworkPDAFactory.h>
#include <aalwines/model/NetworkWeight.h>

#include <aalwines/query/parsererrors.h>
#include <aalwines/utils/stopwatch.h>
#include <aalwines/utils/outcome.h>
#include <aalwines/Verifier.h>

#include <aalwines/model/builders/NetworkParsing.h>

#include <boost/program_options.hpp>

#include <iostream>
#include <string>
#include <vector>
#include <fstream>

namespace po = boost::program_options;
using namespace aalwines;
using namespace pdaaal;

/*
 TODO:
 * fix error-handling
 * improve on throws/exception-types
 * fix slots
 */
int main(int argc, const char** argv)
{
    po::options_description opts;
    opts.add_options()
            ("help,h", "produce help message");

    NetworkParsing parser("Input Options");
    po::options_description output("Output Options");
    Verifier verifier("Verification Options");
    
    bool print_dot = false;
    bool print_dot_topo = false;
    bool print_net = false;
    bool print_info = false;
    bool no_parser_warnings = false;
    bool silent = false;
    bool no_timing = false;
    std::string json_destination, json_pretty_destination, json_topo_destination;

    output.add_options()
            ("dot", po::bool_switch(&print_dot), "A dot output will be printed to cout when set.")
            ("dot-topo", po::bool_switch(&print_dot_topo), "A dot output of the topology will be printed to cout when set.")
            ("net", po::bool_switch(&print_net), "A json output of the network will be printed to cout when set.")
            ("info", po::bool_switch(&print_info), "Print info/stats about the network.")
            ("disable-parser-warnings,W", po::bool_switch(&no_parser_warnings), "Disable warnings from parser.")
            ("silent,s", po::bool_switch(&silent), "Disables non-essential output (implies -W).")
            ("no-timing", po::bool_switch(&no_timing), "Disables timing output")
            ("write-json", po::value<std::string>(&json_destination), "Write the network in the AalWiNes MPLS Network format to the given file.")
            ("write-json-pretty", po::value<std::string>(&json_pretty_destination), "Pretty print the network in the AalWiNes MPLS Network format to the given file.")
            ("write-json-topology", po::value<std::string>(&json_topo_destination), "Write the topology of the network in the AalWiNes MPLS Network format to the given file.")
    ;

    std::string query_file;
    std::string weight_file;
    verifier.add_options()
            ("query,q", po::value<std::string>(&query_file), "A file containing valid queries over the input network.")
            ("weight,w", po::value<std::string>(&weight_file), "A file containing the weight function expression");

    opts.add(parser.options());
    opts.add(output);
    opts.add(verifier.options());

    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, opts), vm);
    po::notify(vm);

    if (vm.count("help")) {
        std::cout << opts << "\n";
        return 1;
    }
    verifier.check_settings();

    if(silent) no_parser_warnings = true;

    auto network = parser.parse(no_parser_warnings);

    if (print_dot) {
        network.print_dot(std::cout);
    }
    if (print_dot_topo) {
        network.print_dot_topo(std::cout);
    }
    if (print_info) {
        network.print_info(std::cout);
    }

    if (!json_destination.empty()) {
        std::ofstream out(json_destination);
        if(out.is_open()) {
            auto j = json::object();
            j["network"] = network;
            out << j << std::endl;
        } else {
            std::cerr << "Could not open --write-json\"" << json_destination << "\" for writing" << std::endl;
            exit(-1);
        }
    }
    if (!json_pretty_destination.empty()) {
        std::ofstream out(json_pretty_destination);
        if(out.is_open()) {
            auto j = json::object();
            j["network"] = network;
            out << j.dump(2) << std::endl;
        } else {
            std::cerr << "Could not open --write-json-pretty\"" << json_pretty_destination << "\" for writing" << std::endl;
            exit(-1);
        }
    }
    if (!json_topo_destination.empty()) {
        std::ofstream out(json_topo_destination);
        if(out.is_open()) {
            auto j = json::object();
            j["network"] = TopologyBuilder::json_topology(network);
            out << j << std::endl;
        } else {
            std::cerr << "Could not open --write-json-topology\"" << json_topo_destination << "\" for writing" << std::endl;
            exit(-1);
        }
    }
    json_stream json_output;
    if (print_net) {
        network.print_json(json_output);
    }
    std::vector<std::string> query_strings;
    if(!query_file.empty()) {
        stopwatch queryparsingwatch;
        Builder builder(network);
        {
            std::ifstream qstream(query_file);
            if (!qstream.is_open()) {
                std::cerr << "Could not open Query-file\"" << query_file << "\"" << std::endl;
                exit(-1);
            }
            try {
                std::string str;
                while(getline(qstream, str)){
                    str.erase(std::remove(str.begin(), str.end(), '\r'), str.end());
                    query_strings.emplace_back(str);
                }
                qstream.close();
                std::ifstream qstream(query_file);
                builder.do_parse(qstream);
                qstream.close();
            }
            catch(base_parser_error& error)
            {
                std::cerr << "Error during parsing:\n" << error << std::endl;
                exit(-1);
            }
        }
        queryparsingwatch.stop();

        std::optional<NetworkWeight::weight_function> weight_fn;
        if (!weight_file.empty()) {
            verifier.check_supports_weight();
            NetworkWeight network_weight;
            {
                std::ifstream wstream(weight_file);
                if (!wstream.is_open()) {
                    std::cerr << "Could not open Weight-file\"" << weight_file << "\"" << std::endl;
                    exit(-1);
                }
                try {
                    weight_fn.emplace(network_weight.parse(wstream));
                    wstream.close();
                } catch (base_error& error) {
                    std::cerr << "Error while parsing weight function:" << error << std::endl;
                    exit(-1);
                } catch (nlohmann::detail::parse_error& error) {
                    std::cerr << "Error while parsing weight function:" << error.what() << std::endl;
                    exit(-1);
                }
            }
        } else {
            weight_fn = std::nullopt;
        }

        if(!no_timing) {
            json_output.entry("network-parsing-time", parser.duration());
            json_output.entry("query-parsing-time", queryparsingwatch.duration());
        }

        json_output.begin_object("answers");
        if (weight_fn) {
            verifier.run(builder, query_strings, json_output, !no_timing, weight_fn.value());
        } else {
            verifier.run(builder, query_strings, json_output, !no_timing);
        }
        json_output.end_object();
    }

    return 0;
}
