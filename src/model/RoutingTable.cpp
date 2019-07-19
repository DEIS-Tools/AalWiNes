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
 * File:   RoutingTable.cpp
 * Author: Peter G. Jensen <root@petergjoel.dk>
 * 
 * Created on July 2, 2019, 3:29 PM
 */

#include "RoutingTable.h"
#include "Router.h"
#include "utils/errors.h"

#include <algorithm>
#include <sstream>
namespace mpls2pda {
RoutingTable::RoutingTable()
{
}

int RoutingTable::parse_weight(rapidxml::xml_node<char>* nh)
{
    auto nhweight = nh->first_node("nh-weight");
    if (nhweight) {
        auto val = nhweight->value();
        auto len = strlen(val);
        if (len > 1 && val[0] == '0' && val[1] == 'x')
            return std::stoull(&(val[2]), nullptr, 16);
        else
            return atoll(val);
    }
    return 0;
}

Interface* RoutingTable::parse_via(Router* parent, rapidxml::xml_node<char>* via)
{
    std::string iname = via->value();
    for (size_t i = 0; i < iname.size(); ++i) {
        if (iname[i] == ' ') {
            iname = iname.substr(0, i);
            break;
        }
    }
    if (iname.find("lsi.") == 0) {
        // self-looping interface
        return parent->get_interface(iname, parent);
    }
    else {
        return parent->get_interface(iname);
    }
}

RoutingTable RoutingTable::parse(rapidxml::xml_node<char>* node, ptrie::map<std::pair<std::string, std::string>>&indirect, Router* parent, std::ostream& warnings)
{
    RoutingTable nr;
    if (node == nullptr)
        return nr;


    if (auto nn = node->first_node("table-name"))
        nr._name = nn->value();

    if (auto tn = node->first_node("address-family")) {
        if (strcmp(tn->value(), "MPLS") != 0) {
            std::stringstream e;
            e << "Not MPLS-type address-family routing-table (\"" << nr._name << "\")" << std::endl;
            throw base_error(e.str());
        }
    }

    auto rule = node->first_node("rt-entry");
    if (rule == nullptr) {
        std::stringstream e;
        e << "no entries in routing-table \"" << nr._name << "\"" << std::endl;
        throw base_error(e.str());
    }

    std::vector<std::pair<std::string, std::pair<size_t, size_t>>> lsi;

    while (rule) {
        std::string tl = rule->first_node("rt-destination")->value();
        nr._entries.emplace_back();
        auto pos = tl.find("(S=0)");
        auto& entry = nr._entries.back();

        if (pos != std::string::npos) {
            if (pos != tl.size() - 5) {
                std::stringstream e;
                e << "expect only (S=0) notation as postfix of <rt-destination> in table " << nr._name << " of router " << parent->name() << std::endl;
                throw base_error(e.str());
            }
            entry._decreasing = true;
            tl = tl.substr(0, pos);
        }
        if (std::all_of(std::begin(tl), std::end(tl), [](auto& c)
            {
                return std::isdigit(c);
            })) {
        entry._top_label = atoll(tl.c_str()) + 1;
    }
        else if (tl == "default") {
            entry._top_label = 0;
        }
        else {
            entry._top_label = (-1 * (int64_t) parent->get_interface(tl)->id()) - 1;
        }

        auto nh = rule->first_node("nh");
        if (nh == nullptr) {
            std::stringstream e;
            e << "no \"nh\" entries in routing-table \"" << nr._name << "\" for \"" << entry._top_label << "\"" << std::endl;
            throw base_error(e.str());
        }
        int cast = 0;

        do {
            entry._rules.emplace_back();
            auto& r = entry._rules.back();
            r._weight = parse_weight(nh);
            auto ops = nh->first_node("nh-type");
            bool skipvia = true;
            rapidxml::xml_node<>* nhid = nullptr;
            if (ops) {
                auto val = ops->value();
                if (strcmp(val, "unilist") == 0) {
                    if (cast != 0) {
                        std::stringstream e;
                        e << "already in cast" << std::endl;
                        throw base_error(e.str());
                    }
                    cast = 1;
                    entry._rules.pop_back();
                    continue;
                }
                else if (strcmp(val, "discard") == 0) {
                    r._type = DISCARD; // Drop
                }
                else if (strcmp(val, "receive") == 0) // check.
                {
                    r._type = RECIEVE; // Drops out of MPLS?
                }
                else if (strcmp(val, "table lookup") == 0) {
                    r._type = ROUTE; // drops out of MPLS?
                }
                else if (strcmp(val, "indirect") == 0) {
                    skipvia = false;
                    // lookup in indirect
                    nhid = nh->first_node("nh-index");
                    if (!nhid) {
                        std::stringstream e;
                        e << "expected nh-index of indirect";
                        throw base_error(e.str());
                    }
                }
                else if (strcmp(val, "unicast") == 0) {
                    skipvia = false;
                    // no ops, though.
                }
                else {
                    std::string ostr = ops->value();
                    r.parse_ops(ostr);
                    skipvia = false;
                }
            }

            auto via = nh->first_node("via");
            if (via && strlen(via->value()) > 0) {
                if (skipvia) {
                    warnings << "warning: found via \"" << via->value() << "\" in \"" << nr._name << "\" for \"" << entry._top_label << "\"" << std::endl;
                    warnings << "\t\tbut got type expecting no via: " << ops->value() << std::endl;
                }

                r._via = parse_via(parent, via);
            }
            else if (!skipvia && indirect.size() > 0) {
                if (nhid) {
                    auto val = nhid->value();
                    auto alt = indirect.exists((const unsigned char*) val, strlen(val));
                    if (!alt.first) {
                        std::stringstream e;
                        e << "Could not lookup indirect : " << val << std::endl;
                        e << "\ttype : " << ops->value() << std::endl;
                        throw base_error(e.str());
                    }
                    else {
                        auto& d = indirect.get_data(alt.second);
                        r._via = parent->get_interface(d.first);
                    }
                }
                else {
                    warnings << "warning: found no via in \"" << nr._name << "\" for \"" << entry._top_label << "\"" << std::endl;
                    warnings << "\t\tbut got type: " << ops->value() << std::endl;
                    warnings << std::endl;
                }
            }
        }
        while ((nh = nh->next_sibling("nh")));
        rule = rule->next_sibling("rt-entry");
    }
    std::sort(nr._entries.begin(), nr._entries.end());
    std::stringstream e;
    bool some = false;
    for (size_t i = 1; i < nr._entries.size(); ++i) {
        if (nr._entries[i - 1] == nr._entries[i]) {
            some = true;
            e << "nondeterministic routing-table found, dual matches on " << nr._entries[i]._top_label << " for router " << parent->name() << std::endl;
        }
    }
    if (some) {
        throw base_error(e.str());
    }
    return nr;
}

void RoutingTable::forward_t::parse_ops(std::string& ostr)
{
    auto pos = ostr.find("(top)");
    if (pos != std::string::npos) {
        if (pos != ostr.size() - 5) {
            std::stringstream e;
            e << "expected \"(top)\" predicate at the end of <nh-type> only." << std::endl;
            throw base_error(e.str());
        }
        ostr = ostr.substr(0, pos);
    }
    // parse ops
    bool parse_label = false;
    _ops.emplace_back();
    for (size_t i = 0; i < ostr.size(); ++i) {
        if (ostr[i] == ' ') continue;
        if (ostr[i] == ',') continue;
        if (!parse_label) {
            if (ostr[i] == 'S') {
                _ops.back()._op = SWAP;
                i += 4;
                parse_label = true;
            }
            else if (ostr[i] == 'P') {
                if (ostr[i + 1] == 'u') {
                    _ops.back()._op = PUSH;
                    parse_label = true;
                    i += 4;
                }
                else if (ostr[i + 1] == 'o') {
                    _ops.back()._op = POP;
                    i += 2;
                    _ops.emplace_back();
                    continue;
                }
            }

            if (parse_label) {
                while (i < ostr.size() && ostr[i] == ' ') {
                    ++i;
                }
                if (i != ostr.size() && std::isdigit(ostr[i])) {
                    size_t j = i;
                    while (j < ostr.size() && std::isdigit(ostr[j])) {
                        ++j;
                    }
                    auto n = ostr.substr(i, (j - i));
                    _ops.back()._label = std::atoi(n.c_str());
                    i = j;
                    parse_label = false;
                    _ops.emplace_back();
                    continue;
                }
            }
            std::stringstream e;
            e << "unexpected operation type \"" << (&ostr[i]) << "\"." << std::endl;
            throw base_error(e.str());
        }
    }
    _ops.pop_back();
}

bool RoutingTable::overlaps(const RoutingTable& other, Router& parent, std::ostream& warnings) const
{
    auto oit = other._entries.begin();
    for (auto& e : _entries) {

        while (oit != std::end(other._entries) && (*oit) < e)
            ++oit;
        if (oit == std::end(other._entries))
            return false;
        if ((*oit) == e) {
            if (e._rules.size() == 1 && oit->_rules.size() == 1 &&
                e._rules[0]._type == oit->_rules[0]._type && oit->_rules[0]._type != MPLS)
                continue;
            warnings << "\t\tOverlap on label ";
            entry_t::print_label(e._top_label, warnings);
            warnings << " for router " << parent.name() << std::endl;
            return true;
        }
    }
    return false;
}

bool RoutingTable::entry_t::operator<(const entry_t& other) const
{
    if (other._decreasing == _decreasing)
        return _top_label < other._top_label;
    return _decreasing < other._decreasing;
}

bool RoutingTable::entry_t::operator==(const entry_t& other) const
{
    return _decreasing == other._decreasing && _top_label == other._top_label;
}

void RoutingTable::action_t::print_json(std::ostream& s) const
{
    switch (_op) {
    case SWAP:
        s << "{\"swap\":";
        print_label(_label, s);
        s << "}";
        break;
    case PUSH:
        s << "{\"push\":";
        print_label(_label, s);
        s << "}";
        break;
    case POP:
        s << "\"pop\"";
        break;
    }
}

void RoutingTable::action_t::print_label(int64_t label, std::ostream& s, bool quoted)
{
    if(quoted) s << "\"";
    if(label == 0) s << "default";
    else if(label < 0) s << "I" << (-(label+1));
    else if(label > 0) s << "L" << (label-1);
    if(quoted) s << "\"";
}

void RoutingTable::entry_t::print_json(std::ostream& s) const
{
    print_label(_top_label, s);
    s << ":\n";
    s << "\t[";
    for (size_t i = 0; i < _rules.size(); ++i) {
        if (i != 0)
            s << ",";
        s << "\n\t\t";
        _rules[i].print_json(s);
    }
    s << "\n\t]";
}

void RoutingTable::entry_t::print_label(label_t label, std::ostream& s)
{
    s << "\"";
    if (label == 0)
        s << "default";
    else if (label > 0)
        s << 'l' << (label - 1);
    else
        s << 'i' << ((label + 1)*-1);
    s << "\"";
}

void RoutingTable::forward_t::print_json(std::ostream& s) const
{
    s << "{";
    s << "\"weight\":" << _weight;
    if (_via) {
        s << ", \"via\":" << _via->id() << ", \"ops\":[";
        for (size_t i = 0; i < _ops.size(); ++i) {
            if (i != 0)
                s << ", ";
            _ops[i].print_json(s);
        }
        s << "]";
    }
    else
        s << ",  \"drop\":true";

    s << "}";
}

void RoutingTable::print_json(std::ostream& s) const
{
    s << "\t{\n";
    for (size_t i = 0; i < _entries.size(); ++i) {
        if (i != 0)
            s << ",\n";
        s << "\t";
        _entries[i].print_json(s);
    }
    s << "\n\t}";
}
}