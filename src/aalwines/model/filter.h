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
 * File:   filter.h
 * Author: Peter G. Jensen <root@petergjoel.dk>
 *
 * Created on July 17, 2019, 5:52 PM
 */

#ifndef FILTER_H
#define FILTER_H

#include <unordered_set>
#include <functional>
#include <string>

namespace aalwines {
    // we are going to use some lazy function-combination here.
    // probably not the fastest or prettiest, but seems to be the easiest.
    struct filter_t {
        std::function<bool(const std::string&)> _from = [](const std::string&){ return true; };
        std::function<bool(const std::string&, const std::string&, const std::string&)> _link =
        [](const std::string&, const std::string&, const std::string&){ return true; };
        filter_t operator&&(const filter_t& other);
    };
}

#endif /* FILTER_H */

