#ifndef DICT_KEY_VALUE_HPP
#define DICT_KEY_VALUE_HPP

#include <utility>

namespace io {

namespace detail {

// inspired/taken from libc++
template <class Key, class Value>
union key_value {
    using key_type = Key;
    using mapped_type = Value;
    using value_type = std::pair<const key_type, mapped_type>;
    using internal_value_type = std::pair<key_type, mapped_type>;

    value_type const_view;
    internal_value_type view;

    // template <typename... Args>
    // key_value(Args&&... args)
    //     : const_view(std::forward<Args>(args)...) {}

    // clang has a problem with the variadic version above
    // Not sure whether this is a bug or not
    key_value() : const_view() {}
    key_value(value_type&& other) : const_view(std::move(other)) {}
    key_value(const value_type& other) : const_view(other) {}
    template <typename K, typename V>
    key_value(K&& k, V&& v)
        : const_view(std::forward<K>(k), std::forward<V>(v)) {}

    key_value(const key_value& other) : const_view(other.const_view) {}

    key_value(key_value&& other)
    noexcept(std::is_nothrow_move_constructible<internal_value_type>::value)
        : view(std::move(other.view)) {}

    key_value& operator=(const key_value& other) {
        view = other.const_view;
        return *this;
    }

    key_value& operator=(key_value&& other) noexcept(
        std::is_nothrow_move_assignable<internal_value_type>::value) {
        view = std::move(other.view);
        return *this;
    }

    ~key_value() { const_view.~value_type(); }
};

} // detail

} // namespace io

#endif
