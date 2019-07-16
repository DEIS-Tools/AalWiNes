//
// Created by Peter G. Jensen on 10/15/18.
//

#include "Builder.h"
#include "errors.h"
#include "Scanner.h"
#include "Dispatcher.h"

#include <model/CostVar.h>
#include <model/Module.h>

#include <filesystem>
#include <fstream>
#include <cassert>

namespace std {

    ostream &operator<<(ostream &os, const std::vector<std::string> &vec) {
        bool first = true;
        for (auto &e : vec) {
            if (!first) os << ", ";
            os << e;
            first = false;
        }
        return os;
    }

    ostream &operator<<(ostream &os, const std::unordered_map<CostVar *, int> &map) {
        os << "{\n";
        for (auto &e : map)
            os << "\t[" << *e.first << "] = " << e.second << "\n";
        os << "}\n";
        return os;
    }

    ostream &operator<<(ostream &os, const std::pair<int, int> &vals) {
        os << "[" << vals.first << ", " << vals.second << "]";
        return os;
    }
}

namespace mpls2pda {

    int Builder::do_parse(std::istream &stream) {
        Scanner scanner(&stream, *this);
        Parser parser(scanner, *this);
        scanner.set_debug(false);
        parser.set_debug_level(false);
        return parser.parse();
    }

    void
    Builder::error(const location &l, const std::string &m) {
        throw base_error(l, m);
    }

    void
    Builder::error(const std::string &m) {
        throw base_error(m);
    }

}

