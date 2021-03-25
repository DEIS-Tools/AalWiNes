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
    void RoutingTable::add_rule(label_t top_label, action_t op, Interface* via, size_t weight) {
        add_rule(top_label, forward_t({op}, via, weight));
    }
    void RoutingTable::forward_t::add_action(action_t action) { // This function applies reductions to the operation list when adding new actions.
        if (_ops.empty()) {
            _ops.push_back(action);
            return;
        }
        switch (action._op) {
            case op_t::PUSH:
                if (_ops.back()._op == op_t::POP) {
                    _ops.pop_back();
                    add_action(action_t(op_t::SWAP, action._op_label));
                    return;
                }
                break;
            case op_t::SWAP: // TODO: case SWAP and case POP doesn't seem happen, so do we want to keep it?
                if (_ops.back()._op == op_t::SWAP || _ops.back()._op == op_t::PUSH) {
                    _ops.back()._op_label = action._op_label;
                    return;
                }
                break;
            case op_t::POP:
                if (_ops.back()._op == op_t::SWAP) {
                    _ops.pop_back();
                    add_action(action_t(op_t::POP));
                    return;
                }
                break;
        }
        _ops.push_back(action);
    }
    void RoutingTable::add_failover_entries(const Interface* failed_inf, Interface* backup_inf, label_t failover_label) {
        for (auto& e : _entries) {
            std::vector<forward_t> new_rules;
            for (const auto& f : e._rules) {
                if (f._via == failed_inf) {
                    new_rules.emplace_back(f._ops, backup_inf, f._priority + 1);
                    new_rules.back().add_action(action_t(op_t::PUSH, failover_label));
                }
            }
            e._rules.insert(e._rules.end(), new_rules.begin(), new_rules.end());
        }
    }
    void RoutingTable::entry_t::add_to_outgoing(const Interface *outgoing, action_t action) {
        for (auto&& f : _rules) {
            if (f._via == outgoing) {
                f.add_action(action);
            }
        }
    }
    void RoutingTable::add_to_outgoing(const Interface* outgoing, action_t action) {
        for (auto& e : _entries) {
            e.add_to_outgoing(outgoing, action);
        }
    }

    void RoutingTable::merge(const RoutingTable& other) {
        assert(std::is_sorted(other._entries.begin(), other._entries.end()));
        assert(std::is_sorted(_entries.begin(), _entries.end()));
        auto iit = _entries.begin();
        for (const auto & e : other._entries) {
            while (iit != std::end(_entries) && (*iit) < e) ++iit;
            if (iit == std::end(_entries)) {
                iit = _entries.insert(iit, e);
            } else if ((*iit) == e) {
                bool legal_merge = true;
                for (const auto& rule : e._rules) { // TODO: Consider sorted rules for faster merge.
                    // Already exists
                    if (std::find(iit->_rules.begin(), iit->_rules.end(), rule) != iit->_rules.end()) continue;
                    // Different traffic engineering group
                    if (std::find_if(iit->_rules.begin(), iit->_rules.end(),
                                     [w = rule._priority](const forward_t& r){ return r._priority == w; })
                        == iit->_rules.end()) {
                        iit->_rules.push_back(rule);
                    } else {
                        legal_merge = false;
                        break;
                    }
                }
                if (legal_merge) continue;
                assert(false); // TODO: Figure out what to do here!
                iit->_rules.insert(iit->_rules.end(), e._rules.begin(), e._rules.end());
            } else {
                assert(e < (*iit));
                iit = _entries.insert(iit, e);
            }
        }
        assert(std::is_sorted(_entries.begin(), _entries.end()));
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
    bool RoutingTable::forward_t::operator==(const forward_t& other) const {
        return _via == other._via && _priority == other._priority
               && _ops.size() == other._ops.size() && std::equal(_ops.begin(), _ops.end(), other._ops.begin());
    }
    bool RoutingTable::forward_t::operator!=(const forward_t& other) const {
        return !(*this == other);
    }
    bool RoutingTable::action_t::operator==(const action_t& other) const {
        return _op == other._op && _op_label == other._op_label;
    }
    bool RoutingTable::action_t::operator!=(const action_t& other) const {
        return !(*this == other);
    }

    std::pair<pdaaal::op_t,size_t> RoutingTable::forward_t::first_action(const std::function<std::pair<bool,size_t>(const label_t&)>& label_abstraction) const {
        if (_ops.empty()) {
            return std::make_pair(pdaaal::NOOP, std::numeric_limits<uint32_t>::max());
        } else {
            return _ops[0].convert_to_pda_op(label_abstraction);
        }
    }
    std::pair<pdaaal::op_t,size_t> RoutingTable::action_t::convert_to_pda_op(const std::function<std::pair<bool,size_t>(const label_t&)>& label_abstraction) const {
        pdaaal::op_t op = pdaaal::POP;
        size_t op_label = std::numeric_limits<uint32_t>::max();
        switch (_op) {
            case RoutingTable::op_t::PUSH:
                op = pdaaal::PUSH;
                assert(label_abstraction(_op_label).first);
                op_label = label_abstraction(_op_label).second;
                break;
            case RoutingTable::op_t::SWAP:
                op = pdaaal::SWAP;
                assert(label_abstraction(_op_label).first);
                op_label = label_abstraction(_op_label).second;
                break;
            case op_t::POP:
                break;
        }
        return std::make_pair(op, op_label);
    }
    std::pair<pdaaal::op_t,Query::label_t> RoutingTable::forward_t::first_action() const {
        if (_ops.empty()) {
            return std::make_pair(pdaaal::NOOP, Query::label_t());
        } else {
            return _ops[0].convert_to_pda_op();
        }
    }
    std::pair<pdaaal::op_t,Query::label_t> RoutingTable::action_t::convert_to_pda_op() const {
        pdaaal::op_t op;
        Query::label_t op_label;
        switch (_op) {
            case RoutingTable::op_t::POP:
                op = pdaaal::POP;
                break;
            case RoutingTable::op_t::PUSH:
                op = pdaaal::PUSH;
                op_label = _op_label;
                break;
            case RoutingTable::op_t::SWAP:
                op = pdaaal::SWAP;
                op_label = _op_label;
                break;
        }
        return std::make_pair(op, op_label);
    }

    void RoutingTable::action_t::print_json(std::ostream& s, bool quote, bool use_hex) const
    {
        switch (_op) {
        case op_t::SWAP:
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
                s << (quote ? "\"" : "");
            }
            s << "}";
            break;
        case op_t::PUSH:
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
                s << (quote ? "\"" : "");
            }
            s << "}";
            break;
        case op_t::POP:
            if (quote) s << "\"";
            s << "pop";
            if (quote) s << "\"";
            break;
        }
    }
    json RoutingTable::action_t::to_json() const {
        json result;
        switch (_op) {
            case op_t::SWAP: {
                std::stringstream s;
                s << _op_label;
                result["swap"] = s.str();
                return result;
            }
            case op_t::PUSH: {
                std::stringstream s;
                s << _op_label;
                result["push"] = s.str();
                break;
            }
            case op_t::POP: {
                result = "pop";
                break;
            }
        }
        return result;
    }

    void RoutingTable::entry_t::print_json(std::ostream& s) const {
        if (ignores_label()) {
            s << "\"null\"";
        } else {
            print_label(_top_label, s);
        }
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

    void RoutingTable::entry_t::print_label(label_t label, std::ostream& s, bool quote) {
        if (quote) s << "\"";
        s << label;
        if (quote) s << "\"";
    }

    void RoutingTable::forward_t::print_json(std::ostream& s, bool use_hex) const {
        s << "{";
        s << "\"weight\":" << _priority;
        if (_via) {
            s << ", \"via\":";
            if (use_hex) {
                s << _via->id();
            } else {
                s << "\"" << _via->get_name() << "\"";
            }
        }
        else
            s << ",  \"drop\":true";
        if (!_ops.empty()) {
            s << ", \"ops\":[";
            for (size_t i = 0; i < _ops.size(); ++i) {
                if (i != 0)
                    s << ", ";
                _ops[i].print_json(s, true, use_hex);
            }
            s << "]";
        }
        s << "}";
    }

    json RoutingTable::forward_t::to_json() const {
        json rule = json::object();
        rule["weight"] = _priority;
        assert(_via);
        rule["via"] = _via->get_name();
        if (!_ops.empty()) {
            rule["ops"] = json::array();
            for (const auto& op : _ops) {
                rule["ops"].push_back(op.to_json());
            }
        }
        return rule;
    }

    void RoutingTable::print_json(std::ostream& s) const {
        s << "\t{\n";
        for (size_t i = 0; i < _entries.size(); ++i) {
            if (i != 0)
                s << ",\n";
            s << "\t";
            _entries[i].print_json(s);
        }
        s << "\n\t}";
    }

    void RoutingTable::sort() {
        std::sort(std::begin(_entries), std::end(_entries));
        std::sort(std::begin(_my_interfaces), std::end(_my_interfaces));
        std::sort(std::begin(_out_interfaces), std::end(_out_interfaces));
    }
    void RoutingTable::sort_rules() {
        for (auto& entry : _entries) {
            std::sort(entry._rules.begin(), entry._rules.end(), [](const auto& a, const auto& b){ return a._priority < b._priority; });
        }
    }

    bool RoutingTable::empty() const {
        return _entries.empty();
    }

    const std::vector<RoutingTable::entry_t>& RoutingTable::entries() const {
        return _entries;
    }

    std::ostream& operator<<(std::ostream& s, const RoutingTable::forward_t& fwd) {
        fwd.print_json(s);
        return s;
    }

    std::ostream& operator<<(std::ostream& s, const RoutingTable::entry_t& entry) {
        s << entry._top_label << " ";
        entry.print_json(s);
        return s;
    }

    void RoutingTable::update_interfaces(const std::function<Interface*(const Interface*)>& update_fn) {
        for (auto& entry : _entries) {
            for (auto& rule : entry._rules) {
                rule._via = update_fn(rule._via);
            }
        }
    }

    void RoutingTable::pre_process_rules(std::ostream& log) {
        // If a rule uses a link that it (due to its priority) also assumes to be disabled, then we can remove that rule.
        // Update the _priority of the rule to be the size of the set of failed links needed for that rule to be active.
        for (auto& entry : _entries) {
            if (entry._rules.empty()) continue;

            auto log_removal = [&entry,&log](const forward_t& forward){
                log << "Removing rule: \"";
                (entry.ignores_label() ? (log << "null") : (log << entry._top_label)) << "\" : ";
                forward.print_json(log, false);
                log << std::endl;
            };

            std::unordered_set<const Interface*> higher_priority_interfaces;
            std::unordered_set<const Interface*> temp;
            size_t last_priority = 0;
            // This is like std::remove_if, but for stateful predicate.
            auto first = entry._rules.begin();
            auto last = entry._rules.end();
            for (; first != last; ++first) {
                assert(first->_priority >= last_priority);
                if (first->_priority > last_priority) {
                    higher_priority_interfaces = temp;
                    last_priority = first->_priority;
                }
                first->_priority = higher_priority_interfaces.size(); // Set priority to the size of the smallest set of failed links that makes this rule (*first) be active.
                temp.emplace(first->_via);
                if (higher_priority_interfaces.count(first->_via) > 0) { // Remove *first
                    log_removal(*first);
                    for(auto i = first; ++i != last; ) {
                        assert(i->_priority >= last_priority);
                        if (i->_priority > last_priority) {
                            higher_priority_interfaces = temp;
                            last_priority = i->_priority;
                        }
                        i->_priority = higher_priority_interfaces.size();
                        temp.emplace(i->_via);
                        if (higher_priority_interfaces.count(i->_via) == 0) {
                            *first++ = std::move(*i);
                        } else {
                            log_removal(*i);
                        }
                    }
                    break;
                }
            }
            entry._rules.erase(first, last);
        }
    }

}
