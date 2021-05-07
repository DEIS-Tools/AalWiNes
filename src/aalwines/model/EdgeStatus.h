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
 *  Copyright Morten K. Schou
 */

/* 
 * File:   EdgeStatus.h
 * Author: Morten K. Schou <morten@h-schou.dk>
 *
 * Created on 05-03-2021.
 */

#ifndef AALWINES_EDGESTATUS_H
#define AALWINES_EDGESTATUS_H

#include <cassert>

namespace aalwines {

    class EdgeStatus {
        std::vector<const Interface*> _failed;
        std::vector<const Interface*> _used;
    public:
        // TODO: Optimization, once _failed.size() == max_failures, we no longer need to keep track of _used.
        EdgeStatus() = default;

        EdgeStatus(std::vector<const Interface*> failed, std::vector<const Interface*> used)
        : _failed(std::move(failed)), _used(std::move(used)) {};

        bool operator==(const EdgeStatus& other) const {
            return _failed == other._failed && _used == other._used;
        }

        bool operator!=(const EdgeStatus& other) const {
            return !(*this == other);
        }

        [[nodiscard]] std::optional <EdgeStatus>
        next_edge_state(const RoutingTable::entry_t& entry, const RoutingTable::forward_t& forward, size_t max_failures) const {
            assert(std::find(entry._rules.begin(), entry._rules.end(), forward) != entry._rules.end());
            auto lbf = std::lower_bound(_failed.begin(), _failed.end(), forward._via);
            if (lbf != _failed.end() && *lbf == forward._via)
                return std::nullopt; // Cannot use edge that was already assumed to by failed

            std::vector<const Interface*> new_failed;
            for (const auto& other : entry._rules) {
                if (other._priority < forward._priority) {
                    new_failed.push_back(other._via); // Rules with higher priority (smaller numeric value) must fail for this forwarding rule to be applicable.
                }
            }
            if (new_failed.empty()) { // Speed up special (but common) case.
                return EdgeStatus(_failed, add_to_set(_used, forward._via));
            }
            std::sort(new_failed.begin(), new_failed.end());
            new_failed.erase(std::unique(new_failed.begin(), new_failed.end()), new_failed.end());

            std::vector<const Interface*> next_failed;
            std::set_union(new_failed.begin(), new_failed.end(), _failed.begin(), _failed.end(),
                           std::back_inserter(next_failed));
            if (next_failed.size() > max_failures) return std::nullopt;

            auto next_used = add_to_set(_used, forward._via);
            assert(is_disjoint(_failed, next_used));
            if (!is_disjoint(new_failed, next_used)) return std::nullopt; // Failed and used must be disjoint.

            return EdgeStatus(std::move(next_failed), std::move(next_used));
        }

        [[nodiscard]] bool soundness_check(size_t max_failures) const {
            // This checks the soundness of this structure. Can e.g. be used in assert statements in debug mode.
            if (!std::is_sorted(_failed.begin(), _failed.end()))
                return false; // Check that data structure is still sorted vector...
            if (!std::is_sorted(_used.begin(), _used.end())) return false;
            if (std::adjacent_find(_failed.begin(), _failed.end()) != _failed.end())
                return false; //... with unique elements. I.e. a set.
            if (std::adjacent_find(_used.begin(), _used.end()) != _used.end()) return false;
            if (_failed.size() > max_failures) return false; // Check |F| <= k
            if (!is_disjoint(_failed, _used)) return false; // Check F \cap U == \emptyset
            return true;
        }

    private:
        static std::vector<const Interface*>
        add_to_set(const std::vector<const Interface*>& set, const Interface* elem) {
            auto lb = std::lower_bound(set.begin(), set.end(), elem);
            if (lb != set.end() && *lb == elem) { // Elem is already in the set.
                return set;
            }
            std::vector<const Interface*> next_set;
            next_set.reserve(set.size() + 1);
            next_set.insert(next_set.end(), set.begin(), lb);
            next_set.push_back(elem);
            next_set.insert(next_set.end(), lb, set.end());
            return next_set;
        }

        template<typename T>
        static bool is_disjoint(const std::vector<T>& a, const std::vector<T>& b) {
            // Inspired by: https://stackoverflow.com/a/29123390
            assert(std::is_sorted(a.begin(), a.end()));
            assert(std::is_sorted(b.begin(), b.end()));
            auto it_a = a.begin();
            auto it_b = b.begin();
            while (it_a != a.end() && it_b != b.end()) {
                if (*it_a < *it_b) {
                    it_a = std::lower_bound(++it_a, a.end(), *it_b);
                } else if (*it_b < *it_a) {
                    it_b = std::lower_bound(++it_b, b.end(), *it_a);
                } else {
                    return false;
                }
            }
            return true;
        }
    };
}

#endif //AALWINES_EDGESTATUS_H
