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
 * File:   NetworkSAXHandler.h
 * Author: Morten K. Schou <morten@h-schou.dk>
 *
 * Created on 19-10-2020.
 */

#ifndef AALWINES_NETWORKSAXHANDLER_H
#define AALWINES_NETWORKSAXHANDLER_H

#include <json.hpp>
#include <aalwines/model/Network.h>
#include <iostream>
#include <fstream>

using json = nlohmann::json;

namespace aalwines {

    class NetworkSAXHandler {
    private:
        enum class keys : uint32_t { none, unknown, network, network_name, routers, links, router_name, router_alias,
            location, latitude, longitude, interfaces, interface_name, interface_names, routing_table,
            table_label, entry_out, priority, ops, weight, pop, swap, push,
            from_router, from_interface, to_router, to_interface, bidirectional, link_weight };
        friend constexpr std::ostream& operator<<( std::ostream&, keys key );

    public:
        struct context {
            enum class context_type : uint32_t { unknown, initial, network, link_array, link, router_array, router,
                                                 location, router_alias_array, interface_array, interface, interface_names_array, routing_table,
                                                 entry_array, entry, operation_array, operation };
            friend constexpr std::ostream& operator<<(std::ostream&, context_type type );
            enum key_flag : uint32_t {
                NO_FLAGS = 0,
                FLAG_1 = 1,
                FLAG_2 = 2,
                FLAG_3 = 4,
                FLAG_4 = 8,
                // Required values for each object type.
                REQUIRES_0 = NO_FLAGS,
                REQUIRES_1 = FLAG_1,
                REQUIRES_2 = FLAG_1 | FLAG_2,
                REQUIRES_3 = FLAG_1 | FLAG_2 | FLAG_3,
                REQUIRES_4 = FLAG_1 | FLAG_2 | FLAG_3 | FLAG_4
            };
            static constexpr std::array<key_flag,4> all_flags{key_flag::FLAG_1, key_flag::FLAG_2, key_flag::FLAG_3, key_flag::FLAG_4};

            context_type type;
            key_flag values_left;

            constexpr void got_value(key_flag value) {
                values_left = static_cast<context::key_flag>(static_cast<uint32_t>(values_left) & ~static_cast<uint32_t>(value));
            }
            [[nodiscard]] constexpr bool needs_value(key_flag value) const {
                return static_cast<uint32_t>(value) == (static_cast<uint32_t>(values_left) & static_cast<uint32_t>(value));
            }
            [[nodiscard]] constexpr bool missing_keys() const {
                return values_left != NO_FLAGS;
            }
            static constexpr keys get_key(context_type context_type, key_flag flag);
        };
    private:
        constexpr static context unknown_context = {context::context_type::unknown, context::NO_FLAGS };
        constexpr static context initial_context = {context::context_type::initial, context::REQUIRES_1 };
        constexpr static context network_context = {context::context_type::network, context::REQUIRES_3 };
        constexpr static context link_array = {context::context_type::link_array, context::NO_FLAGS };
        constexpr static context link_context = {context::context_type::link, context::REQUIRES_4 };
        constexpr static context router_array = {context::context_type::router_array, context::NO_FLAGS };
        constexpr static context router_context = {context::context_type::router, context::REQUIRES_2 };
        constexpr static context router_alias_array = {context::context_type::router_alias_array, context::NO_FLAGS };
        constexpr static context interface_array = {context::context_type::interface_array, context::NO_FLAGS };
        constexpr static context interface_context = {context::context_type::interface, context::REQUIRES_2 };
        constexpr static context interface_names_array = {context::context_type::interface_names_array, context::NO_FLAGS };
        constexpr static context location_context = {context::context_type::location, context::REQUIRES_2 };
        constexpr static context table_context = {context::context_type::routing_table, context::REQUIRES_0 };
        constexpr static context entry_array = {context::context_type::entry_array, context::NO_FLAGS };
        constexpr static context entry_context = {context::context_type::entry, context::REQUIRES_3 };
        constexpr static context operation_array = {context::context_type::operation_array, context::NO_FLAGS };
        constexpr static context operation_context = {context::context_type::operation, context::REQUIRES_1 };

        std::stack<context> context_stack;
        keys last_key = keys::none;
        std::ostream& errors;

        string_map<Router*> router_map;
        std::vector<std::unique_ptr<Router>> routers;
        std::vector<const Interface*> all_interfaces;
        std::string network_name;

        bool routers_parsed = false;

        // Router
        Router* current_router = nullptr;
        std::string current_router_name; // The primary name should be added last, so we will temporarily keep it here.
        double latitude = 0;
        double longitude = 0;

        // Interface
        std::vector<Interface*> current_interfaces;
        RoutingTable* current_table = nullptr;
        std::unordered_set<const Interface*> current_table_out_interfaces;
        std::unordered_set<std::string> forward_constructed_interfaces;

        // Entry
        Interface* via = nullptr;
        size_t priority = 0;
        uint32_t weight = 0;
        std::vector<RoutingTable::action_t> ops;

        // Link
        std::string current_from_router_name;
        std::string current_from_interface_name;
        std::string current_to_router_name;
        std::string current_to_interface_name;
        bool current_link_bidirectional = false;
        uint32_t current_link_weight = std::numeric_limits<uint32_t>::max();

        std::vector<std::tuple<std::string,std::string,std::string,std::string,bool,uint32_t>> links; // Used if links are parsed before routers.
        bool pair_link(const std::string& from_router_name, const std::string& from_interface_name,
                       const std::string& to_router_name, const std::string& to_interface_name,
                       bool bidirectional, uint32_t link_weight);
        bool add_router_name(const std::string& value);
        bool add_interface_name(const std::string& value);
        template <context::context_type type, context::key_flag flag, keys key, keys... alternatives> bool handle_key();
    public:
        using number_integer_t = typename json::number_integer_t;
        using number_unsigned_t = typename json::number_unsigned_t;
        using number_float_t = typename json::number_float_t;
        using string_t = typename json::string_t;
        using binary_t = typename json::binary_t;

        explicit NetworkSAXHandler(std::ostream& errors = std::cerr) : errors(errors) {};

        Network get_network() {
            Network network(std::move(router_map), std::move(routers), std::move(all_interfaces));
            network.add_null_router();
            network.name = network_name;
            return network;
        }

        bool null();
        bool boolean(bool value);
        bool number_integer(number_integer_t value);
        bool number_unsigned(number_unsigned_t value);
        bool number_float(number_float_t value, const string_t& /*unused*/);
        bool string(string_t& value);
        bool binary(binary_t& val);
        bool start_object(std::size_t /*unused*/ = std::size_t(-1));
        bool key(string_t& key);
        bool end_object();
        bool start_array(std::size_t /*unused*/ = std::size_t(-1));
        bool end_array();
        bool parse_error(std::size_t location, const std::string& last_token, const nlohmann::detail::exception& e);
    };

    class FastJsonBuilder {
    public:
        static Network parse(std::istream& stream, std::ostream& warnings, json::input_format_t format = json::input_format_t::json) {
            std::stringstream es; // For errors;
            NetworkSAXHandler my_sax(es);
            if (!json::sax_parse(stream, &my_sax, format)) {
                throw base_error(es.str());
            }
            return my_sax.get_network();
        }

        static Network parse(const std::string& network_file, std::ostream& warnings, json::input_format_t format = json::input_format_t::json) {
            std::ifstream stream(network_file);
            if (!stream.is_open()) {
                std::stringstream es;
                es << "error: Could not open file : " << network_file << std::endl;
                throw base_error(es.str());
            }
            return parse(stream, warnings, format);
        }
    };

}

#endif //AALWINES_NETWORKSAXHANDLER_H
