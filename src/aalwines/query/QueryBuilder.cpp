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
#include "aalwines/utils/parsing.h"

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

    int Builder::do_parse(std::istream &stream)
    {
        Scanner scanner(&stream, *this);
        Parser parser(scanner, *this);
        scanner.set_debug(false);
        parser.set_debug_level(false);
        return parser.parse();
    }

    void Builder::error(const location &l, const std::string &m)
    {
        throw base_parser_error(l, m);
    }

    void Builder::error(const std::string &m)
    {
        throw base_parser_error(m);
    }

    Builder::Builder(Network& network)
    : _network(network)
    {
    }

    void Builder::path_mode()
    {
        _pathmode = true;
    }

    void Builder::label_mode()
    {
        _pathmode = false;
    }

    Builder::labelset_t Builder::any_ip()
    {
        auto r1 = expand_labels(Query::label_t::any_ip4);
        auto r2 = expand_labels(Query::label_t::any_ip6);
        r1.insert(r2.begin(), r2.end());
        r1.insert(Query::label_t::any_ip4);
        r1.insert(Query::label_t::any_ip6);
        return r1;
    }

    Builder::labelset_t Builder::any_mpls()
    {
        return expand_labels(Query::label_t::any_mpls);
    }

    Builder::labelset_t Builder::any_sticky()
    {
        return expand_labels(Query::label_t::any_sticky_mpls);
    }


    Builder::labelset_t Builder::expand_labels(Query::label_t label)
    {
        return get_labels(label.value(), label.mask(), label.type());
    }

    Builder::labelset_t Builder::match_ip4(int i1, int i2, int i3, int i4, int mask)
    {
        if(mask > 32)
        {
            throw type_error("IPv4 mask can be at most 32");
        }
        for(auto n : {i1, i2, i3, i4})
        {
            if(n > 256 || n < 0)
            {
                throw type_error("IPv4 format contains only numbers between 0 and 255.");
            }
        }
        uint32_t val;
        uint8_t* ptr = (uint8_t*)&val;
        ptr[0] = i4;
        ptr[1] = i3;
        ptr[2] = i2;
        ptr[3] = i1;
        val = ((val << mask) >> mask); // canonize
        return expand_labels(Query::label_t(Query::IP4, static_cast<uint8_t>(mask), val));
    }

    Builder::labelset_t Builder::match_ip6(int i1, int i2, int i3, int i4, int i5, int i6, int i7, int i8, int mask)
    {
        if(mask > 64)
        {
            throw type_error("IPv6 mask can be at most 64");
        }
        for(auto n : {i1, i2, i3, i4, i5, i6, i7, i8})
        {
            if(n > 256 || n < 0)
            {
                throw type_error("IPv6 format contains only numbers between 00 and FF.");
            }
        }
        uint64_t val;
        uint8_t* ptr = (uint8_t*)&val;
        ptr[0] = i8;
        ptr[1] = i7;
        ptr[2] = i6;
        ptr[3] = i5;
        ptr[4] = i4;
        ptr[5] = i3;
        ptr[6] = i2;
        ptr[7] = i1;
        val = ((val << mask) >> mask); // canonize
        return expand_labels(Query::label_t(Query::IP6, static_cast<uint8_t>(mask), val));
    }

    filter_t Builder::match_exact(const std::string& str) const
    {
        filter_t res;
        if(!_post && !_link)
        {
            res._from = [str](const char* name)
            {
                return str == name;
            };
        }
        else
        {
            bool is_link = _link, is_post = _post;
            res._link = [is_link,is_post,str](const char* fname, const char* tname, const char* trname){
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

    filter_t Builder::match_re(std::string&& re) const
    {
        filter_t res;
        boost::regex regex(re);
        if(!_post)
        {
            res._from = [regex](const char* name)
            {
                return boost::regex_match(name, regex);
            };
        }
        else
        {
            bool is_link = _link, is_post = _post;
            res._link = [regex,is_link,is_post](const char* fname, const char* tname, const char* trname){
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
    
    Builder::labelset_t Builder::find_label(uint64_t label, uint64_t mask)
    {
        return get_labels(label, mask, _sticky ? Query::STICKY_MPLS : Query::MPLS, !_expand);
    }
    
    filter_t Builder::discard_id()
    {
        throw base_error("discard matching is not yet implemented");
        filter_t res;
        /*res._link = [](const char* fname, const char* tname, const char* trname){
            if(strcmp(fname, "!") == 0) return true;
            return false;
        };*/
        return res;
    }

    filter_t Builder::routing_id()
    {
        throw base_error("routing matching is not yet implemented");
        filter_t res;
        /*res._link = [](const char* fname, const char* tname, const char* trname){
            if(strcmp(fname, "^") == 0) return true;
            return false;
        };*/
        return res;
    }

    Builder::labelset_t Builder::filter(filter_t& f)
    {
        return _network.interfaces(f);
    }

    Builder::labelset_t Builder::filter_and_merge(filter_t& f, labelset_t& r)
    {
        auto res = filter(f);
        res.insert(r.begin(), r.end());
        return res;
    }

    Builder::labelset_t Builder::all_labels()
    {
        if(_label_cache.empty())
        {
            std::unordered_set<Query::label_t> res;
            res.insert(Query::label_t::unused_ip4);
            res.insert(Query::label_t::unused_ip6);
            res.insert(Query::label_t::unused_mpls);
            res.insert(Query::label_t::unused_sticky_mpls);
            res.insert(Query::label_t::any_ip);
            res.insert(Query::label_t::any_ip4);
            res.insert(Query::label_t::any_ip6);
            for (const auto& r : _network.routers()) {
                for (const auto& inf : r->interfaces()) {
                    for (const auto& e : inf->table().entries()) {
                        res.insert(e._top_label);
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

    Builder::labelset_t Builder::get_labels(const labelset_t& all_labels, uint64_t label, uint64_t mask, Query::type_t type, bool exact)
    {
        Query::label_t lbl(type, mask, label);
        std::unordered_set<Query::label_t> res;
        for (auto& pr : all_labels) {
            if(pr == Query::label_t::unused_ip4 ||
               pr == Query::label_t::unused_ip6 ||
               pr == Query::label_t::unused_mpls ||
               pr == Query::label_t::unused_sticky_mpls ||
               pr == Query::label_t::any_ip ||
               pr == Query::label_t::any_ip4 ||
               pr == Query::label_t::any_ip6) continue;
            if (lbl.overlaps(pr))
                res.insert(pr);
        }
        if(res.empty())
        {
            switch (type) {
                case Query::IP4:
                    res.emplace(Query::label_t::unused_ip4);
                    break;
                case Query::IP6:
                    res.emplace(Query::label_t::unused_ip6);
                    break;
                case Query::STICKY_MPLS:
                    res.emplace(Query::label_t::unused_sticky_mpls);
                    break;
                case Query::MPLS:
                    res.emplace(Query::label_t::unused_mpls);
                    break;
                default:
                    throw base_error("Unknown expansion");
            }
        }
        if(!exact)
        {
            switch (type) {
                case Query::IP4:
                    res.emplace(Query::label_t::any_ip4);
                    break;
                case Query::IP6:
                    res.emplace(Query::label_t::any_ip6);
                    break;
                default:
                    break;
            }
        }
#ifndef NDEBUG
        for(const auto& r : res)
        {
            assert(r.mask() == 0 || type == Query::IP6 || type == Query::IP4 || type == Query::ANYIP);
        }
#endif
        return res;
    }


}

