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
#ifndef PROJECT_BUILDER_H
#define PROJECT_BUILDER_H

#include "location.hh"
#include "pdaaal/model/NFA.h"
#include "model/Query.h"
#include "model/Network.h"
#include "model/filter.h"

#include <string>
#include <sstream>
#include <map>
#include <stack>
#include <unordered_map>
#include <memory>
#include <vector>
#include <cassert>
#include <queue>
#include <functional>
#include <unordered_set>

namespace std {

    template<typename X>
    ostream &operator<<(ostream &os, const std::vector<X> &vec) {
        bool first = true;
        os << "[";
        for (auto &e : vec) {
            if (!first) os << ", ";
            else os << *e;
            first = false;
        }
        os << "]";
        return os;
    }

    ostream &operator<<(ostream &os, const std::vector<std::string> &vec);
}

namespace mpls2pda {

    class Builder {
    public:
        Builder(Network& network);
        // Return 0 on success.
        int do_parse(std::istream &stream);

	// Building
	void path_mode();
	void label_mode();
        using labelset_t = std::unordered_set<Query::label_t>;
        
        // matching on atomics 
        labelset_t filter_and_merge(filter_t&, labelset_t&);
        labelset_t filter(filter_t& f);
        void set_post() { _post = true; }
        void clear_post() { _post = false; }
        void set_link() { _link = true; }
        void clear_link() { _link = false; }
        
        
        filter_t match_re(std::string&& re);
        filter_t match_exact(const std::string& str);
        filter_t routing_id();
        filter_t discard_id();
        labelset_t find_label(uint64_t label, uint64_t mask);
        labelset_t any_ip();
        labelset_t any_mpls();
        labelset_t match_ip4(int i1, int i2, int i3, int i4, int mask);
        labelset_t match_ip6(int i1, int i2, int i3, int i4, int i5, int i6, int i7, int i8, int mask);
        labelset_t expand_labels(Query::label_t label);
        void invert(bool val)
        {
            _inverted = val;
        }
        bool inverted() const { return _inverted; }
        void expand(bool val) { _expand = val; }
        
        // Error handling.
        void error(const location &l, const std::string &m);

        void error(const std::string &m);

        Network& _network;
	location _location;
        std::vector<Query> _result;
        
        // filtering
        labelset_t _links;
        // if we are parsing a path or a stack-type regex
        bool _pathmode = true;
        bool _post = false;
        bool _link = false;
        bool _inverted = false;
        bool _expand = false;
    };
}

#endif //PROJECT_BUILDER_H
