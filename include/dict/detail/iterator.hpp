#ifndef DICT_ITERATOR_HPP
#define DICT_ITERATOR_HPP

#include <vector>
#include <tuple>

#include <boost/iterator/iterator_facade.hpp>

#include "key_value.hpp"

namespace io {

namespace detail {

// iterators
template <typename value_type, typename Iter>
class dict_iterator_base
    : public boost::iterator_facade<dict_iterator_base<value_type, Iter>,
                                    value_type, boost::forward_traversal_tag> {
public:
    dict_iterator_base() : _ptr(), _end() {}
    dict_iterator_base(Iter p, Iter end) : _ptr(p), _end(end) {
        if (_ptr != _end && !std::get<0>(*_ptr)) {
            increment();
        }
    }

    template <typename Other, typename OtherIter>
    dict_iterator_base(const dict_iterator_base<Other, OtherIter>& other)
        : _ptr(other._ptr), _end(other._end) {}

private:
    friend class boost::iterator_core_access;
    template <typename, typename>
    friend class dict_iterator_base;

    void increment() {
        do {
            ++_ptr;
        } while(_ptr != _end && !std::get<0>(*_ptr));

    }

    template <typename OtherValue, typename OtherIter>
    bool equal(const dict_iterator_base<OtherValue, OtherIter>& other) const {
        return this->_ptr == other._ptr;
    }

    value_type& dereference() const { return std::get<1>(*_ptr).const_view; }

    Iter _ptr;
    const Iter _end;
};

template <typename Key, typename Value>
using dict_iterator = dict_iterator_base<
    std::pair<const Key, Value>,
    typename std::vector<std::tuple<bool, key_value<Key, Value>>>::iterator>;

template <typename Key, typename Value>
using const_dict_iterator =
    dict_iterator_base<const std::pair<const Key, Value>,
                       typename std::vector<std::tuple<
                           bool, key_value<Key, Value>>>::const_iterator>;

} // detail

} // namespace io

#endif
