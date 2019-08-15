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
 * File:   Query.h
 * Author: Peter G. Jensen <root@petergjoel.dk>
 *
 * Created on July 16, 2019, 5:38 PM
 */

#ifndef QUERY_H
#define QUERY_H
#include "pdaaal/model/NFA.h"
#include "utils/parsing.h"

#include <functional>
#include <ostream>

namespace mpls2pda {

    class Query {
    public:

        enum mode_t {
            OVER, UNDER, EXACT
        };

        enum type_t {
            ANYIP = 1, IP4 = 2, IP6 = 4, MPLS = 8, INTERFACE = 16
        };

        struct label_t {
            
            type_t _type = MPLS;
            uint8_t _mask = 0;
            uint64_t _value = 0;            
            bool uses_mask() const {
                return _mask != 0 && _type != INTERFACE && _type != ANYIP;
            }
            bool operator<(const label_t& other) const {
                if(_type != other._type)
                    return _type < other._type;
                if(_mask != other._mask)
                    return _mask < other._mask;
                return _value < other._value;
            }

            bool operator==(const label_t& other) const {
                return (_value == other._value && _type == other._type && _mask == other._mask);
            }

            bool operator!=(const label_t& other) const {
                return !(*this == other);
            }

            bool operator>=(const label_t& other) const {
                return !(*this < other);
            }

            bool operator>(const label_t& other) const {
                return other < *this;
            }

            bool operator<=(const label_t& other) const {
                return other >= *this;
            }
            
            friend std::ostream & operator << (std::ostream& stream, const label_t& label)
            {
                switch(label._type)
                {
                    case MPLS:
                        stream << "l" << label._value;
                        break;
                    case IP4:
                        write_ip4(stream, label._value);
                        break;
                    case IP6:
                        write_ip6(stream, label._value);
                        break;
                    case INTERFACE:
                        stream << "i" << label._value;
                        break;
                    case ANYIP:
                        stream << "ANY_IP";
                }
                if(label.uses_mask())
                    stream << "/" << (int)label._mask;
                return stream;
            }
            const static label_t unused_mpls;
            const static label_t unused_ip4;
            const static label_t unused_ip6;
            const static label_t any_mpls;
            const static label_t any_ip4;
            const static label_t any_ip6;
        };

        Query() {
        };
        Query(pdaaal::NFA<label_t>&& pre, pdaaal::NFA<label_t>&& path, pdaaal::NFA<label_t>&& post, int lf, mode_t mode);
        Query(Query&&) = default;
        Query(const Query&) = default;
        virtual ~Query() = default;
        Query& operator=(Query&&) = default;
        Query& operator=(const Query&) = default;

        pdaaal::NFA<label_t> & construction() {
            return _prestack;
        }

        pdaaal::NFA<label_t> & destruction() {
            return _poststack;
        }

        pdaaal::NFA<label_t> & path() {
            return _path;
        }

        mode_t approximation() const {
            return _mode;
        }

        int number_of_failures() const {
            return _link_failures;
        }
        void print_dot(std::ostream& out);
    private:
        pdaaal::NFA<label_t> _prestack;
        pdaaal::NFA<label_t> _poststack;
        pdaaal::NFA<label_t> _path;
        int _link_failures = 0;
        mode_t _mode;

    };
}

namespace std {
    template<>
    struct hash<mpls2pda::Query::label_t>
    {
        std::size_t operator()(const mpls2pda::Query::label_t& k) const {
            std::hash<uint64_t> hf;
            return hf(k._value) xor hf((int)k._type) xor hf(k._mask);
        }
    };
    
}
#endif /* QUERY_H */

