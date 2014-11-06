#ifndef DICT_KEY_VALUE_HPP
#define DICT_KEY_VALUE_HPP

#include <utility>

namespace boost {

namespace detail {

// inspired/taken from libc++
template <class Key, class Value>
union key_value {
    typedef Key key_type;
    typedef Value mapped_type;
    typedef std::pair<const key_type, mapped_type> value_type;
    typedef std::pair<key_type, mapped_type> internal_value_type;

    value_type const_view;
    internal_value_type view;

    template <typename... Args>
    key_value(Args&&... args)
        : const_view(std::forward<Args>(args)...) {}

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

} // namespace boost

#endif
