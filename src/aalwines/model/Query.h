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
 * File:   Query.h
 * Author: Peter G. Jensen <root@petergjoel.dk>
 *
 * Created on July 16, 2019, 5:38 PM
 */

#ifndef QUERY_H
#define QUERY_H
#include <pdaaal/NFA.h>
#include <pdaaal/ptrie_interface.h>
#include "aalwines/utils/errors.h"

#include <functional>
#include <ostream>
#include <ptrie/ptrie.h>

namespace aalwines {

    class Query {
    public:

        enum mode_t {
            OVER, UNDER, DUAL, EXACT
        };

        using label_t = size_t;
        static constexpr label_t unused_label() noexcept { return std::numeric_limits<size_t>::max() - 2; }
        static constexpr label_t bottom_of_stack() noexcept { return std::numeric_limits<size_t>::max() - 1; }
        static constexpr label_t wildcard_label() noexcept { return std::numeric_limits<size_t>::max(); }

        Query() = default;
        Query(pdaaal::NFA<label_t>&& pre, pdaaal::NFA<label_t>&& path, pdaaal::NFA<label_t>&& post, size_t lf, mode_t mode)
        : _prestack(std::move(pre)), _poststack(std::move(post)), _path(std::move(path)), _link_failures(lf), _mode(mode) {
            _prestack.concat(pdaaal::NFA<label_t>(std::unordered_set<label_t>{Query::bottom_of_stack()}));
            _poststack.concat(pdaaal::NFA<label_t>(std::unordered_set<label_t>{Query::bottom_of_stack()}));
        };

        pdaaal::NFA<label_t>& construction() {
            return _prestack;
        }

        pdaaal::NFA<label_t>& destruction() {
            return _poststack;
        }

        pdaaal::NFA<label_t>& path() {
            return _path;
        }
        [[nodiscard]] const pdaaal::NFA<label_t>& path() const {
            return _path;
        }
        
        void set_approximation(mode_t approx) {
            _mode = approx;
        }
        
        [[nodiscard]] mode_t approximation() const {
            return _mode;
        }

        [[nodiscard]] size_t number_of_failures() const {
            return _link_failures;
        }

        void compile_nfas() {
            _prestack.compile();
            _poststack.compile();
            _path.compile();
        }

        void print_dot(std::ostream& out);
    private:
        pdaaal::NFA<label_t> _prestack;
        pdaaal::NFA<label_t> _poststack;
        pdaaal::NFA<label_t> _path;
        size_t _link_failures = 0;
        mode_t _mode;
    };
}

#endif /* QUERY_H */

