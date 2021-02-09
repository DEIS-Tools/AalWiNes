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
// Created by Peter G. Jensen on 10/15/18.
//

#include "QueryBuilder.h"
#include "parsererrors.h"
#include "Scanner.h"

#include <cassert>
#include <iostream>
#include <boost/regex.hpp>


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

namespace aalwines
{

    int Builder::do_parse(std::istream &stream) {
        Scanner scanner(&stream, *this);
        Parser parser(scanner, *this);
        scanner.set_debug(false);
        parser.set_debug_level(false);
        return parser.parse();
    }

    void Builder::error(const location &l, const std::string &m) {
        throw base_parser_error(l, m);
    }

    void Builder::error(const std::string &m) {
        throw base_parser_error(m);
    }

    filter_t Builder::match_exact(const std::string& str) const {
        filter_t res;
        if(!_post && !_link) {
            res._from = [str](const std::string& name) {
                return str == name;
            };
        } else {
            bool is_link = _link, is_post = _post;
            res._link = [is_link,is_post,str](const std::string& fname, const std::string& tname, const std::string& trname) {
                if(!is_link)
                    return str == trname;
                else if(!is_post)
                    return str == fname;
                else
                    return str == tname;
            };            
        }
        return res;
    }

    filter_t Builder::match_re(std::string&& re) const {
        filter_t res;
        boost::regex regex(re);
        if(!_post) {
            res._from = [regex](const std::string& name)
            {
                return boost::regex_match(name, regex);
            };
        } else {
            bool is_link = _link, is_post = _post;
            res._link = [regex,is_link,is_post](const std::string& fname, const std::string& tname, const std::string& trname){
                if(!is_link)
                    return regex_match(trname, regex);
                else if(!is_post)
                    return regex_match(fname, regex);
                else
                    return regex_match(tname, regex);
            };
        }
        return res;
    }

    Builder::labelset_t Builder::filter(filter_t& f) {
        return _network.interfaces(f);
    }

    Builder::labelset_t Builder::filter_and_merge(filter_t& f, labelset_t& r) {
        auto res = filter(f);
        res.insert(r.begin(), r.end());
        return res;
    }


    Builder::labelset_t Builder::all_labels() {
        if(_label_cache.empty()) {
            std::unordered_set<Query::label_t> res;
            res.insert(Query::unused_label()); // This label will 'match' any label in the query that is not present in the network.
            res.insert(Query::bottom_of_stack()); // This label is used in the PDA construction to represent the bottom of the stack.
            for (const auto& r : _network.routers()) {
                for (const auto& inf : r->interfaces()) {
                    for (const auto& e : inf->table().entries()) {
                        if (!e.ignores_label()) res.insert(e._top_label);
                        for (const auto& f : e._rules) {
                            for (const auto& o : f._ops) {
                                switch (o._op) {
                                    case RoutingTable::op_t::SWAP:
                                    case RoutingTable::op_t::PUSH:
                                        res.insert(o._op_label);
                                    default:
                                        break;
                                }
                            }
                        }
                    }
                }
            }
            _label_cache = res;
        }
        return _label_cache;
    }

}

