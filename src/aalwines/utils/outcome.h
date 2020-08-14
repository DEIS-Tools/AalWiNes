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
 *  Copyright Peter G. Jensen, all rights reserved.
 */

/* 
 * File:   outcome.h
 * Author: Peter G. Jensen <root@petergjoel.dk>
 *
 * Created on October 7, 2019, 1:51 PM
 */

#ifndef OUTCOME_H
#define OUTCOME_H

#include <ostream>
#include <json.hpp>

namespace utils {
    enum class outcome_t { YES, NO, MAYBE };

    std::ostream& operator<<(std::ostream& os, const outcome_t& outcome) {
        switch (outcome) {
            case outcome_t::YES:
                os << "YES";
                break;
            case outcome_t::NO:
                os << "NO";
                break;
            case outcome_t::MAYBE:
                os << "MAYBE";
                break;
        }
        return os;
    }

    using json = nlohmann::json;
    inline void from_json(const json & j, outcome_t& outcome) {
        if (j.is_null()) {
            outcome = outcome_t::MAYBE;
        } else if (j.is_boolean()) {
            if (j.get<bool>()) {
                outcome = outcome_t::YES;
            } else {
                outcome = outcome_t::NO;
            }
        } else {
            throw base_error("error: outcome must be either true, false or null.");
        }
    }
    inline void to_json(json & j, const outcome_t& outcome) {
        switch (outcome) {
            case outcome_t::YES:
                j = true;
                break;
            case outcome_t::NO:
                j = false;
                break;
            case outcome_t::MAYBE:
                j = nullptr;
                break;
        }
    }
}

#endif /* OUTCOME_H */

