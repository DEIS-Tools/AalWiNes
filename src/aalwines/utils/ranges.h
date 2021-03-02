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
 * File:   ranges.h
 * Author: Morten K. Schou <morten@h-schou.dk>
 *
 * Created on 01-03-2021.
 */

#ifndef AALWINES_RANGES_H
#define AALWINES_RANGES_H

#include <utility>
#include <vector>

namespace aalwines::utils {

// Given that we are stuck with C++17 for the moment, we implement some usefull ranges here.
// This is not intended to match the C++20 ranges, it is just based on what was needed.

// SingletonRange<Elem>                Contains a single element
// FilterRange<InnerRange>             Apply a filter (predicate) to elements in the inner range
// VectorRange<InnerRange, ValueType>  Transform each element in inner range to a vector of elements and iterate through the concatenated (flattened) range of these vectors.
// VariantRange<Range1, Ranges...>     Variant type for ranges. Can be either of the types Range1, Ranges...


    template<typename Elem>
    struct SingletonRange {
        explicit constexpr SingletonRange(const Elem& elem) : _begin(elem) {};
        explicit constexpr SingletonRange(Elem&& elem) : _begin(std::move(elem)) {};
        template<typename... Args>
        explicit constexpr SingletonRange(Args&& ... args) : _begin(std::forward<Args>(args)...) {}

        struct sentinel;
        struct iterator {
            using value_type = Elem;
            using pointer = const value_type*;
            using reference = const value_type&;

            constexpr iterator() = default;
            explicit constexpr iterator(const Elem& elem) : _elem(elem) {};
            explicit constexpr iterator(Elem&& elem) : _elem(std::move(elem)) {};
            template<typename... Args>
            explicit constexpr iterator(Args&& ... args) : _elem(std::forward<Args>(args)...) {}

            pointer operator->() const {
                return &_elem;
            }
            reference operator*() const& {
                return _elem;
            }
            value_type&& operator*() && {
                return std::move(_elem);
            }
            iterator& operator++() noexcept {
                _end = true;
                return *this;
            }
            bool operator==(const sentinel& rhs) const noexcept { return _end; }
            bool operator!=(const sentinel& rhs) const noexcept { return !(*this == rhs); }

        private:
            bool _end = false;
            Elem _elem;
        };
        struct sentinel {
            bool operator==(const iterator& it) const noexcept { return it == *this; }
            bool operator!=(const iterator& it) const noexcept { return !(it == *this); }
        };
        using value_type = typename iterator::value_type;

        [[nodiscard]] iterator begin() const& {
            return _begin;
        }
        [[nodiscard]] iterator&& begin() && {
            return std::move(_begin);
        }
        [[nodiscard]] sentinel end() const {
            return sentinel();
        }

    private:
        iterator _begin;
    };

    template<typename InnerRange>
    struct FilterRange {
    public:
        struct sentinel;

        struct iterator {
            using inner_iterator = std::remove_reference_t<decltype(std::declval<InnerRange&&>().begin())>;
            using inner_sentinel = std::remove_reference_t<decltype(std::declval<InnerRange&&>().end())>;
            using value_type = typename inner_iterator::value_type;
            using filter_fn = std::function<bool(const value_type&)>;
        private:
            inner_sentinel _inner_end;
            inner_iterator _inner_it;
            filter_fn _filter;
            value_type _current;
        public:
            iterator(InnerRange&& range, filter_fn&& filter) noexcept
            : _inner_end(range.end()), _inner_it(std::move(range).begin()), _filter(std::move(filter)) {
                while (!(is_end() || is_valid())) { // Go forward to the first valid element (or the end).
                    ++_inner_it;
                }
            };

            value_type operator*() const& {
                return _current; // We have already computed it when checking is_valid, so we use the stored value.
            }
            value_type&& operator*() && {
                return std::move(_current);
            }
            iterator& operator++() noexcept {
                do {
                    ++_inner_it;
                } while (!(is_end() || is_valid()));
                return *this;
            }

            bool operator==(const sentinel& rhs) const noexcept { return is_end(); }
            bool operator!=(const sentinel& rhs) const noexcept { return !(*this == rhs); }

        private:
            bool is_valid() {
                _current = *_inner_it;
                return _filter(_current);
            }
            [[nodiscard]] bool is_end() const {
                return _inner_it == _inner_end;
            }
        };
        struct sentinel {
            bool operator==(const iterator& it) const noexcept { return it == *this; }
            bool operator!=(const iterator& it) const noexcept { return !(it == *this); }
        };

        using value_type = typename iterator::value_type;
    private:
        using filter_fn = typename iterator::filter_fn;
    public:
        FilterRange(InnerRange&& range, filter_fn&& filter) : _begin(std::move(range), std::move(filter)) {};

        [[nodiscard]] iterator begin() const& { return _begin; }
        [[nodiscard]] iterator&& begin() && { return std::move(_begin); }
        [[nodiscard]] sentinel end() const { return sentinel(); }

    private:
        iterator _begin;
    };

    template<typename InnerRange, typename FilterFn>
    FilterRange(InnerRange&& range, FilterFn&& filter) -> FilterRange<InnerRange>;


    template<typename InnerRange, typename ValueType>
    struct VectorRange {
    public:
        struct sentinel;
        struct iterator {
            using inner_iterator = std::remove_reference_t<decltype(std::declval<InnerRange&&>().begin())>;
            using inner_sentinel = std::remove_reference_t<decltype(std::declval<InnerRange&&>().end())>;
            using inner_value_type = typename inner_iterator::value_type;
            using value_type = ValueType;
            using transform_fn = std::function<std::vector<value_type>(const inner_value_type&)>;
        private:
            inner_sentinel _inner_end;
            inner_iterator _inner_it;
            transform_fn _transform;
            std::vector<value_type> _current_vector;
            size_t _current_vector_i = 0;
        public:
            iterator(InnerRange&& range, transform_fn&& transform) noexcept
            : _inner_end(range.end()), _inner_it(std::move(range).begin()), _transform(std::move(transform)),
              _current_vector(_inner_it == _inner_end ? std::vector<value_type>() : _transform(*_inner_it)) {
                while (!(is_end() || is_valid())) { // Go forward to the first valid element (or the end).
                    next();
                }
            };

            value_type operator*() const& {
                return _current_vector.at(_current_vector_i);
            }

            iterator& operator++() noexcept {
                do {
                    next();
                } while (!(is_end() || is_valid()));
                return *this;
            }

            bool operator==(const sentinel& rhs) const noexcept { return is_end(); }
            bool operator!=(const sentinel& rhs) const noexcept { return !(*this == rhs); }

        private:
            void next() {
                if (_current_vector_i < _current_vector.size()) {
                    ++_current_vector_i;
                } else {
                    ++_inner_it;
                    if (!is_end()) {
                        _current_vector = _transform(*_inner_it);
                        _current_vector_i = 0;
                    }
                }
            }
            bool is_valid() {
                return _current_vector_i < _current_vector.size();
            }

            [[nodiscard]] bool is_end() const {
                return _inner_it == _inner_end;
            }
        };
        struct sentinel {
            bool operator==(const iterator& it) const noexcept { return it == *this; }
            bool operator!=(const iterator& it) const noexcept { return !(it == *this); }
        };

        using value_type = typename iterator::value_type;
    private:
        using transform_fn = typename iterator::transform_fn;
    public:
        VectorRange(InnerRange&& range, transform_fn&& transform) : _begin(std::move(range), std::move(transform)) {};
        [[nodiscard]] iterator begin() const& { return _begin; }
        [[nodiscard]] iterator&& begin() && { return std::move(_begin); }
        [[nodiscard]] sentinel end() const { return sentinel(); }
    private:
        iterator _begin;
    };

    // These template don't really work/help, but meh..
    template<typename InnerRange, typename TransformFn>
    VectorRange(InnerRange&& range, TransformFn&& transform) -> VectorRange<InnerRange, typename TransformFn::result_type::value_type>;
    template<typename InnerRange, typename ValueType>
    VectorRange(InnerRange&& range, std::function<std::vector<ValueType>(const typename std::remove_reference_t<decltype(std::declval<InnerRange>().begin())>::value_type&)>&& transform) -> VectorRange<InnerRange, ValueType>;


    namespace detail {
        // Get index of a type in a variant. Only works if the variant has unique types.
        // Based on https://stackoverflow.com/questions/52303316/get-index-by-type-in-stdvariant
        // and https://stackoverflow.com/questions/53651609/why-does-this-get-index-implementation-fail-on-vs2017
        template <typename> struct tag { };
        template <typename T, typename... Ts> constexpr std::size_t variant_index() { return std::variant<tag<Ts>...>(tag<T>()).index(); }
        template <typename T, typename V> struct get_index;
        template <typename T, typename... Ts>
        struct get_index<T, std::variant<Ts...>> : std::integral_constant<size_t, variant_index<T, Ts...>()> { };
        template <typename T, typename V> constexpr size_t get_index_v = get_index<T,V>::value;
    }

    template<typename Range1, typename... Ranges>
    struct VariantRange {
        static_assert((std::is_same_v<typename Range1::iterator::value_type, typename Ranges::iterator::value_type> && ...));
        struct sentinel;
        struct iterator {
        private:
            using iterator_variant_t = std::variant<std::remove_reference_t<decltype(std::declval<Range1>().begin())>, std::remove_reference_t<decltype(std::declval<Ranges>().begin())>...>;
            using sentinel_variant_t = std::variant<std::remove_reference_t<decltype(std::declval<Range1>().end())>, std::remove_reference_t<decltype(std::declval<Ranges>().end())>...>;
            static constexpr size_t variant_size = std::variant_size_v<iterator_variant_t>;
        public:
            using value_type = typename Range1::iterator::value_type;

            constexpr iterator() = default;
            template<typename Range>
            constexpr explicit iterator(Range&& range)
            : _inner(std::forward<Range>(range).begin()), _end(std::forward<Range>(range).end()) {
                static_assert((std::is_same_v<Range, Range1> || ... || std::is_same_v<Range, Ranges>));
                static_assert(!std::is_same_v<Range,iterator>);
            }

            template<std::size_t I, typename Range>
            constexpr iterator(std::in_place_index_t<I>, Range&& range)
            : _inner(std::in_place_index<I>, std::forward<Range>(range).begin()),
              _end(std::in_place_index<I>, std::forward<Range>(range).end()) { }

            value_type operator*() const& {
                // std::visit is slow, so optimize for current/standard use case.
                if constexpr (variant_size == 1) {
                    return *std::get<0>(_inner);
                } else if constexpr (variant_size == 2) {
                    if (_inner.index() == 0) {
                        return *std::get<0>(_inner);
                    } else {
                        return *std::get<1>(_inner);
                    }
                } else if constexpr (variant_size == 3) {
                    switch (_inner.index()) {
                        case 0:
                            return *std::get<0>(_inner);
                        case 1:
                            return *std::get<1>(_inner);
                        case 2:
                        default:
                            return *std::get<2>(_inner);
                    }
                } else {
                    return std::visit([](auto&& inner){ return *inner; }, _inner);
                }
            }
            iterator& operator++() noexcept {
                // std::visit is slow, so optimize for current/standard use case.
                if constexpr (variant_size == 1) {
                    ++std::get<0>(_inner);
                } else if constexpr (variant_size == 2) {
                    if (_inner.index() == 0) {
                        ++std::get<0>(_inner);
                    } else {
                        ++std::get<1>(_inner);
                    }
                } else if constexpr (variant_size == 3) {
                    switch (_inner.index()) {
                        case 0:
                            ++std::get<0>(_inner);
                            break;
                        case 1:
                            ++std::get<1>(_inner);
                            break;
                        case 2:
                        default:
                            ++std::get<2>(_inner);
                            break;
                    }
                } else {
                    std::visit([](auto&& inner){ ++inner; }, _inner);
                }
                return *this;
            }
            bool operator==(const sentinel& rhs) const noexcept {
                // std::visit is slow, so optimize for current/standard use case.
                assert(_inner.index() == _end.index());
                if constexpr (variant_size == 1) {
                    return std::get<0>(_inner) == std::get<0>(_end);
                } else if constexpr (variant_size == 2) {
                    return _inner.index() == 0 ? std::get<0>(_inner) == std::get<0>(_end) : std::get<1>(_inner) == std::get<1>(_end);
                } else if constexpr (variant_size == 3) {
                    switch (_inner.index()) {
                        case 0:
                            return std::get<0>(_inner) == std::get<0>(_end);
                        case 1:
                            return std::get<1>(_inner) == std::get<1>(_end);
                        case 2:
                        default:
                            return std::get<2>(_inner) == std::get<2>(_end);
                    }
                } else {
                    return (_inner.index() == _end.index()) &&
                           std::visit([this](auto&& inner) { // Use std::visit to find type of _inner, and on compile-time find corresponding index and use it to std::get value of _end.
                               using T = std::decay_t<decltype(inner)>;
                               return inner == std::get<detail::get_index_v<T,iterator_variant_t>>(_end); // Use operator== of the actual values of _inner and _end.
                           }, _inner);
                }
            }
            bool operator!=(const sentinel& rhs) const noexcept { return !(*this == rhs); }

        private:
            iterator_variant_t _inner;
            sentinel_variant_t _end;
        };
        struct sentinel {
            bool operator==(const iterator& it) const noexcept { return it == *this; }
            bool operator!=(const iterator& it) const noexcept { return !(it == *this); }
        };
        using value_type = typename iterator::value_type;

        constexpr VariantRange() = default;
        template<typename Range>
        constexpr VariantRange(Range&& range)
        : _begin(std::forward<Range>(range)) {
            static_assert((std::is_same_v<Range, Range1> || ... || std::is_same_v<Range, Ranges>));
            static_assert(!std::is_same_v<Range,VariantRange>);
        }
        template<std::size_t I, typename Range>
        constexpr VariantRange(std::in_place_index_t<I>, Range&& range)
        : _begin(std::in_place_index<I>, std::forward<Range>(range)) { }

        [[nodiscard]] iterator begin() const& {
            return _begin;
        }
        [[nodiscard]] iterator&& begin() && {
            return std::move(_begin);
        }
        [[nodiscard]] sentinel end() const {
            return sentinel();
        }
    private:
        iterator _begin;
    };




}
#endif //AALWINES_RANGES_H
