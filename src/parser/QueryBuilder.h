//
// Created by Peter G. Jensen on 10/15/18.
//
#ifndef PROJECT_BUILDER_H
#define PROJECT_BUILDER_H

#include "location.hh"

#include <string>
#include <sstream>
#include <map>
#include <stack>
#include <unordered_map>
#include <memory>
#include <vector>
#include <cassert>

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

    ostream &operator<<(ostream &os, const std::unordered_map<adpp::CostVar *, int> &map);

    ostream &operator<<(ostream &os, const std::vector<std::string> &vec);

    ostream &operator<<(ostream &os, const std::pair<int, int> &vals);
}

namespace mpls2pda {

    class Builder {
    public:
        Builder();
        // Return 0 on success.
        int do_parse(std::istream &stream);

        // Error handling.
        void error(const location &l, const std::string &m);

        void error(const std::string &m);

	location _location;
    };
}

#endif //PROJECT_BUILDER_H
