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

//
// Created by Peter G. Jensen on 10/15/18.
//

#include "QueryBuilder.h"
#include "parsererrors.h"
#include "Scanner.h"

#include <filesystem>
#include <fstream>
#include <cassert>

namespace std
{

    ostream &operator<<(ostream &os, const std::vector<std::string> &vec)
    {
        bool first = true;
        for (auto &e : vec) {
            if (!first) os << ", ";
            os << e;
            first = false;
        }
        return os;
    }
}

namespace mpls2pda
{

    int Builder::do_parse(std::istream &stream)
    {
        Scanner scanner(&stream, *this);
        Parser parser(scanner, *this);
        scanner.set_debug(false);
        parser.set_debug_level(false);
        return parser.parse();
    }

    void
    Builder::error(const location &l, const std::string &m)
    {
        throw base_parser_error(l, m);
    }

    void
    Builder::error(const std::string &m)
    {
        throw base_parser_error(m);
    }

    void Builder::clear_filter()
    {
        _filter.clear();
        _using_filter = false;
    }

    void Builder::set_filter(labelset_t&& labels, bool internal)
    {
        _filter = labels;
        _internal_filter = internal;
        _using_filter = true;
    }

    Builder::Builder(Network& network)
    : _network(network)
    {
    }

    void Builder::link_mode()
    {
        _linkmode = true;
    }

    void Builder::path_mode()
    {
        _linkmode = false;
    }

    Builder::labelset_t Builder::match_ip4(int i1, int i2, int i3, int i4, int mask)
    {
        return {};
    }

    Builder::labelset_t Builder::match_ip6(int i1, int i2, int i3, int i4, int i5, int i6, int mask)
    {
        return {};
    }

    Builder::labelset_t Builder::match_exact(const std::string& str)
    {
        return {};
    }

    Builder::labelset_t Builder::match_re(std::string&& re)
    {
        return {};
    }

    Builder::labelset_t Builder::find_label(uint64_t label, uint64_t mask)
    {
        return {};
    }

    Builder::labelset_t Builder::discard_id()
    {
        return {};
    }

    Builder::labelset_t Builder::ip_id()
    {
        return {};
    }

    Builder::labelset_t Builder::routing_id()
    {
        return {};
    }

}

