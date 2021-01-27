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
#include "aalwines/utils/errors.h"
#include "aalwines/utils/parsing.h"

#include <functional>
#include <ostream>
#include <ptrie/ptrie.h>

namespace aalwines {

    class Query {
    public:

        enum mode_t {
            OVER, UNDER, DUAL, EXACT
        };

        enum type_t {
           ANYIP=2, IP4 = 4, IP6 = 8, MPLS = 16, STICKY = 32, INTERFACE = 64, NONE = 128, STICKY_MPLS = MPLS | STICKY
        };

        struct label_t {
        private:
            type_t _type = NONE;
            uint8_t _mask = 0;
            uint64_t _value = 0;               
        public:
            friend struct ptrie::byte_iterator<label_t>;
            label_t()
            {
                _type = NONE;
                _mask = 0;
                _value = 0;
            }
            explicit label_t(const std::string& s) {
                set_value(s);
            }
            
            type_t type() const { return _type; }
            uint8_t mask() const { return _mask; }
            uint64_t value() const { return _value; }

            void set_value(const std::string& s) {
                if(s.empty()) {
                    set_value(Query::ANYIP, 0, 0);
                } else if(s == "ip4") {
                    set_value(Query::IP6, 0, 32);
                } else if(s == "ip6") {
                    set_value(Query::IP6, 0, 64);
                } else if(s == "ip") {
                    set_value(Query::ANYIP, 0, 64);
                } else if(s == "null") {
                    // TODO: Implement a null label, that matches with an empty stack.
                    set_value(Query::ANYIP, 0, 0); // Dummy for now...
                } else {
                    auto i = s.find_first_of(".:");
                    if (i != std::string::npos) {
                        if (s[i] == '.') {
                            set_value(Query::IP4, parse_ip4(s.c_str()), 0); // TODO: Use std::string and exceptions to parse ip4 and ip6.
                        } else { // s[i] == ':'
                            set_value(Query::IP6, parse_ip6(s.c_str()), 0);
                        }
                        size_t i_mask = s.find('/');
                        if (i_mask != std::string::npos && i_mask + 1 < s.size()){
                            set_mask(std::stoi(s.substr(i_mask + 1)));
                        }
                    } else { // Parse as label
                        bool sticky = s[0] == '$' || s[0] == 's';
                        Query::type_t type = sticky ? Query::STICKY_MPLS : Query::MPLS;
                        set_value(type, std::stoi(s.substr((sticky ? 1 : 0))), 0);
                    }
                }
            }

            void set_type(type_t type)
            {
                set_value(type, _value, _mask);
            }

            void set_value(type_t type, uint64_t val, uint32_t mask)
            {
                _type = type;
                _mask = mask;
                _value = val;
                switch(_type)
                {
                    case ANYIP:
                        _mask = 64;
                        _value = 0;
                        break;
                    case STICKY_MPLS:
                    case MPLS:
                        _mask = std::min<uint8_t>(64, _mask);
                        if(_mask == 64)
                            _value = 0;
                        _value = (_value >> _mask) << _mask;
                        break;
                    case IP4:
                        if(_mask >= 32) {
                            _mask = 64;
                            _value = 0;
                        }
                        _value = std::min<uint64_t>(std::numeric_limits<uint32_t>::max(), _value);
                        _value = (_value >> _mask) << _mask;
                        if(_mask >= 32)
                            _value = 0;
                        break;
                    case IP6:
                        _mask = std::min<uint8_t>(64, _mask);
                        _value = (_value >> _mask) << _mask;
                        if(_mask >= 64)
                            _value = 0;
                        break;
                    default:
                        _mask = 0;
                        break;
                }
            }
            
            label_t(type_t type, uint8_t mask, uint64_t val)
            {
                set_value(type, val, mask);
            }
                  
            void set_mask(uint8_t mask) {
                set_value(_type, _value, mask);
            }
            
            bool uses_mask() const {
                return _mask != 0 && _mask != 64 && _type != INTERFACE && _type != ANYIP;
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
                uint8_t mx = 255;
                switch(label._type)
                {
                    case STICKY_MPLS:
                        stream << "s";
                    case MPLS:
                        if(label._mask >= 64 || label == Query::label_t::unused_mpls || label == Query::label_t::unused_sticky_mpls)
                            stream << "mpls";
                        else
                            stream << ((label._value >> label._mask) << label._mask);
                        break;
                    case IP4:
                        mx = 32;
                        if(label._mask < 32 && label != Query::label_t::unused_ip4)
                            write_ip4(stream, (label._value >> label._mask) << label._mask);
                        else
                        {
                            stream << "ip4";
                            mx = 0;
                        }
                        break;
                    case IP6:
                        mx = 64;
                        if(label._mask < 64 && label != Query::label_t::unused_ip6)
                            write_ip6(stream, (label._value >> label._mask) << label._mask);
                        else
                        {
                            stream << "ip6";
                            mx = 0;
                        }
                        break;
                    case INTERFACE:
                        stream << "i" << label._value;
                        break;
                    case ANYIP:
                        stream << "ip";
                        break;
                    case NONE:
                        stream << "NONE";
                        break;
                    default:
                        throw base_error("Unknown error");
                }
                if(label.uses_mask() && mx != 0)
                {                    
                    stream << "/" << (int)std::min(label._mask, mx);
                }
                return stream;
            }
            
            bool overlaps(const label_t& other) const {
                if(_type == other._type)
                {
                    auto m = std::max(other._mask, _mask);
                    if(_mask >= 64) return true;
                    return (_value >> m) == (other._value >> m);
                }
                if(std::min(_type, other._type) == ANYIP &&
                   (std::max(other._type, _type) == IP4 || std::max(other._type, _type) == IP6))
                    return true;
                return false;
            }
            
            const static label_t unused_mpls;
            const static label_t unused_sticky_mpls;
            const static label_t unused_ip4;
            const static label_t unused_ip6;
            const static label_t any_mpls;
            const static label_t any_sticky_mpls;
            const static label_t any_ip;
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
        
        void set_approximation(mode_t approx)
        {
            _mode = approx;
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
    struct hash<aalwines::Query::label_t>
    {
        std::size_t operator()(const aalwines::Query::label_t& k) const {
            std::hash<uint64_t> hf;
            return hf(k.value()) xor hf((int)k.type()) xor hf(k.mask());
        }
    };
}

namespace ptrie {
    template<>
    struct byte_iterator<aalwines::Query::label_t>
    {
        using label_t = aalwines::Query::label_t;
        static constexpr unsigned char& access(label_t* data, size_t id)
        {
            auto el = id / element_size();
            id = id % element_size();
            switch(id){
                case 0:
                    return (uchar&)data[el]._type;
                case 1:
                    return (uchar&)data[el]._mask;
                default:
                    return ((uchar*)&data[el]._value)[id-2];
            }
        }
        
        static constexpr unsigned char& const_access(const label_t* data, size_t id)
        {
            return access(const_cast<label_t*>(data), id);
        }
        
        static constexpr size_t element_size()
        {
            return sizeof(uint8_t)*2 + sizeof(uint64_t);
        }
        
        static constexpr bool continious()
        {
            return false;
        }
    };
}
#endif /* QUERY_H */

