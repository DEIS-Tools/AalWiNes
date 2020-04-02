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

/* 
 * File:   RoutingTable.cpp
 * Author: Peter G. Jensen <root@petergjoel.dk>
 * 
 * Created on July 2, 2019, 3:29 PM
 */

#include "RoutingTable.h"
#include "Router.h"
#include "aalwines/utils/errors.h"
#include "Network.h"

#include <algorithm>
#include <sstream>
#include <map>
#include <cassert>

namespace aalwines
{

    std::vector<RoutingTable::entry_t>::iterator RoutingTable::insert_entry(label_t top_label) {
        assert(std::is_sorted(_entries.begin(), _entries.end()));
        entry_t entry;
        entry._top_label = top_label;
        auto lb = std::lower_bound(_entries.begin(), _entries.end(), entry);
        if (lb == std::end(_entries) || (*lb) != entry) {
            lb = _entries.insert(lb, entry);
        }
        return lb;
    }
    void RoutingTable::add_rules(label_t top_label, const std::vector<forward_t>& rules) {
        auto it = insert_entry(top_label);
        it->_rules.insert(it->_rules.end(), rules.begin(), rules.end());
    }
    void RoutingTable::add_rule(label_t top_label, const forward_t& rule) {
        insert_entry(top_label)->_rules.push_back(rule);
    }
    void RoutingTable::add_rule(label_t top_label, forward_t&& rule) {
        insert_entry(top_label)->_rules.emplace_back(std::move(rule));
    }
    void RoutingTable::add_rule(RoutingTable::label_t top_label, RoutingTable::action_t op, Interface* via, size_t weight, type_t type) {
        add_rule(top_label, forward_t(type, {op}, via, weight));
    }
    void RoutingTable::add_failover_entries(const Interface* failed_inf, Interface* backup_inf, label_t failover_label) {
        for (auto& e : _entries) {
            std::vector<forward_t> new_rules;
            for (const auto& f : e._rules) {
                if (f._via == failed_inf) {
                    new_rules.emplace_back(f._type, f._ops, backup_inf, f._weight + 1);
                    new_rules.back()._ops.emplace_back(PUSH, failover_label);
                }
            }
            e._rules.insert(e._rules.end(), new_rules.begin(), new_rules.end());
        }
    }
    void RoutingTable::simple_merge(const RoutingTable& other) {
        assert(std::is_sorted(other._entries.begin(), other._entries.end()));
        assert(std::is_sorted(_entries.begin(), _entries.end()));
        auto iit = _entries.begin();
        for (const auto & e : other._entries) {
            while (iit != std::end(_entries) && (*iit) < e) ++iit;
            if (iit == std::end(_entries)) {
                iit = _entries.insert(iit, e);
            } else if ((*iit) == e) {
                if (e._rules.size() == 1 && iit->_rules.size() == 1 &&
                    e._rules[0]._type == iit->_rules[0]._type && iit->_rules[0]._type != MPLS)
                    continue;
                assert(false); // TODO: Figure out what to do here!
                iit->_rules.insert(iit->_rules.end(), e._rules.begin(), e._rules.end());
            } else {
                assert(e < (*iit));
                iit = _entries.insert(iit, e);
            }
        }
        assert(std::is_sorted(_entries.begin(), _entries.end()));
    }

    bool RoutingTable::merge(const RoutingTable& other, Interface& parent, std::ostream& warnings)
    {
        assert(std::is_sorted(other._entries.begin(), other._entries.end()));
        bool all_fine = true;

        assert(std::is_sorted(other._entries.begin(), other._entries.end()));
        assert(std::is_sorted(_entries.begin(), _entries.end()));
        auto iit = _entries.begin();
        for (auto oit = other._entries.begin(); oit != other._entries.end(); ++oit) {
            if (oit->_ingoing != nullptr && oit->_ingoing != &parent) continue;
            auto& e = (*oit);
            while (iit != std::end(_entries) && (*iit) < e)
                ++iit;
            if (iit == std::end(_entries)) {
                iit = _entries.insert(iit, e);
                iit->_ingoing = &parent;
            }
            else if ((*iit) == e) {
                if (e._rules.size() == 1 && iit->_rules.size() == 1 &&
                    e._rules[0]._type == iit->_rules[0]._type && iit->_rules[0]._type != MPLS)
                    continue;
                warnings << "\t\tOverlap on label ";
                entry_t::print_label(e._top_label, warnings);
                warnings << " for router " << parent.source()->name() << std::endl;
                all_fine = false;
                iit->_rules.insert(iit->_rules.end(), e._rules.begin(), e._rules.end());
            }
            else {
                assert(e < (*iit));
                iit = _entries.insert(iit, e);
                iit->_ingoing = &parent;
            }
        }
#ifndef NDEBUG
        for(auto& e : _entries) assert(e._ingoing == &parent);
#endif
        assert(std::is_sorted(_entries.begin(), _entries.end()));
        return all_fine;
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
        return _top_label < other._top_label;
    }

    bool RoutingTable::entry_t::operator==(const entry_t& other) const
    {
        return _top_label == other._top_label;
    }
    bool RoutingTable::entry_t::operator!=(const entry_t& other) const
    {
        return !(*this == other);
    }

    void RoutingTable::action_t::print_json(std::ostream& s, bool quote, bool use_hex, const Network* network) const
    {
        switch (_op) {
        case SWAP:
            s << "{";
            if (quote) s << "\"";
            s << "swap";
            if (quote) s << "\"";
            s << ":";
            if (use_hex)
                entry_t::print_label(_op_label, s, quote);
            else
            {
                s << (quote ? "\"" : "") << _op_label;
                if(network && network->is_service_label(_op_label))
                    s << "^";
                s << (quote ? "\"" : "");
            }
            s << "}";
            break;
        case PUSH:
            s << "{";
            if (quote) s << "\"";
            s << "push";
            if (quote) s << "\"";
            s << ":";
            if (use_hex)
                entry_t::print_label(_op_label, s, quote);
            else
            {
                s << (quote ? "\"" : "") << _op_label;
                if(network && network->is_service_label(_op_label))
                    s << "^";
                s << (quote ? "\"" : "");
            }
            s << "}";
            break;
        case POP:
            if (quote) s << "\"";
            s << "pop";
            if (quote) s << "\"";
            break;
        }
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

    void RoutingTable::entry_t::print_label(label_t label, std::ostream& s, bool quote)
    {
        if (quote) s << "\"";
        switch (label.type()) {
        case Query::STICKY_MPLS:
            s << "s"; // fall through on purpose
        case Query::MPLS:
            s << 'l' << std::dec << label.value() << std::dec;
            assert(label.mask() == 0);
            break;
        case Query::ANYSTICKY:
            s << "s"; // fall through on purpose
        case Query::ANYMPLS:
            s << "am";
            break;
        case Query::IP4:
            s << "ip4" << std::hex << label.value() << "M" << (uint32_t) label.mask() << std::dec;
            assert(label.mask() == 0 || label.value() == std::numeric_limits<uint64_t>::max());
            break;
        case Query::IP6:
            s << "ip6" << std::hex << label.value() << "M" << (uint32_t) label.mask() << std::dec;
            assert(label.mask() == 0 || label.value() == std::numeric_limits<uint64_t>::max());
            break;
        default:
            assert(false);
            throw base_error("Interfaces cannot be pushdown-labels.");
            break;
        }
        if (quote) s << "\"";
    }

    void RoutingTable::forward_t::print_json(std::ostream& s, bool use_hex, const Network* network) const
    {
        s << "{";
        s << "\"weight\":" << _weight;
        if (_via) {
            s << ", \"via\":";
            if (use_hex) {
                s << _via->id();
            }
            else {
                auto iname = _via->source()->interface_name(_via->id());
                s << "\"" << iname.get() << "\"";
            }
        }
        else
            s << ",  \"drop\":true";
        if (_ops.size() > 0) {
            s << ", \"ops\":[";
            for (size_t i = 0; i < _ops.size(); ++i) {
                if (i != 0)
                    s << ", ";
                _ops[i].print_json(s, true, use_hex, network);
            }
            s << "]";
        }
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

    bool RoutingTable::check_nondet(std::ostream& e)
    {
        bool some = false;
        for (size_t i = 1; i < _entries.size(); ++i) {
            if (_entries[i - 1] == _entries[i]) {
                some = true;
                e << "nondeterministic routing-table found, dual matches on " << _entries[i]._top_label << std::endl;
            }
        }
        return !some;
    }

    void RoutingTable::sort()
    {
        std::sort(std::begin(_entries), std::end(_entries));
    }

    bool RoutingTable::empty() const
    {
        return _entries.empty();
    }

    const std::vector<RoutingTable::entry_t>& RoutingTable::entries() const
    {
        return _entries;
    }

    std::ostream& operator<<(std::ostream& s, const RoutingTable::forward_t& fwd)
    {
        fwd.print_json(s);
        return s;
    }

    std::ostream& operator<<(std::ostream& s, const RoutingTable::entry_t& entry)
    {
        s << entry._top_label << " ";
        s << entry._ingoing << " ";
        entry.print_json(s);
        return s;
    }

}
