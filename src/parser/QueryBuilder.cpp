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
#include "utils/parsing.h"

#include <filesystem>
#include <fstream>
#include <cassert>
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

    filter_t Builder::match_ip4(int i1, int i2, int i3, int i4, int mask)
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
        filter_t res;
        if(_link || _post)
        {
            bool is_link = _link;
            res._link = [val, mask,is_link](const char* fname, uint32_t fip4, uint64_t fip6, const char* tname, uint32_t tip4, uint64_t tip6, const char* trname){
                if(!is_link)
                {
                    auto ip = parse_ip4(tname);
                    if(ip == std::numeric_limits<uint32_t>::max()) return false;
                    if((ip << mask) == (val << mask))
                        return true;
                    return false;
                }
                else
                {
                    return (tip4 << mask) == (val << mask); 
                }
            };
        }
        else
        {
            
            res._from = [val, mask](const char* name)
            {
                auto ip = parse_ip4(name);
                    if(ip == std::numeric_limits<uint32_t>::max()) return false;
                    if((ip << mask) == (val << mask))
                        return true;
                    return false;
            };
        }
        return res;
    }

    filter_t Builder::match_ip6(int i1, int i2, int i3, int i4, int i5, int i6, int mask)
    {
        return {};
    }

    filter_t Builder::match_exact(const std::string& str)
    {
        filter_t res;
        if(!_post && !_link)
        {
            res._from = [&](const char* name)
            {
                return (str.compare(name) == 0);
            };
        }
        else
        {
            bool is_link = _link, is_post = _post;
            res._link = [is_link,is_post,str](const char* fname, uint32_t fip4, uint64_t fip6, const char* tname, uint32_t tip4, uint64_t tip6, const char* trname){
                if(!is_link)
                    return (str.compare(trname) == 0);
                else if(!is_post)
                    return (str.compare(fname) == 0);
                else
                    return (str.compare(tname) == 0);
            };            
        }
        return res;
    }

    filter_t Builder::match_re(std::string&& re)
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
            res._link = [regex,is_link,is_post](const char* fname, uint32_t fip4, uint64_t fip6, const char* tname, uint32_t tip4, uint64_t tip6, const char* trname){
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
        return _network.get_labels(label, mask);
    }

    filter_t Builder::discard_id()
    {
        filter_t res;
        res._link = [](const char* fname, uint32_t fip4, uint64_t fip6, const char* tname, uint32_t tip4, uint64_t tip6, const char* trname){
            if(strcmp(fname, "!") == 0) return true;
            return false;
        };
        return res;
    }

    filter_t Builder::routing_id()
    {
        filter_t res;
        res._link = [](const char* fname, uint32_t fip4, uint64_t fip6, const char* tname, uint32_t tip4, uint64_t tip6, const char* trname){
            if(strcmp(fname, "^") == 0) return true;
            return false;
        };
        return res;
    }

    Builder::labelset_t Builder::ip_labels(filter_t& filter)
    {
        return {_network.ip_labels(filter)};
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

}

