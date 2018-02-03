#ifndef DICT_ITERATOR_HPP
#define DICT_ITERATOR_HPP

#include <vector>
#include <tuple>

#include <boost/iterator/iterator_facade.hpp>

#include "key_value.hpp"
#include "entry.hpp"

namespace io {

namespace detail {

// iterators
template <typename value_type, typename Iter>
class dict_iterator_base
    : public boost::iterator_facade<dict_iterator_base<value_type, Iter>,
                                    value_type, boost::forward_traversal_tag> {
public:
    dict_iterator_base() : _meta_ptr(nullptr), _ptr(), _end() {}
    dict_iterator_base(const std::uint8_t* m, Iter p, Iter end)
        : _meta_ptr(m),  _ptr(p), _end(end) {
        if (_ptr != _end && *_meta_ptr == 0) {
            increment();
        }
    }
    dict_iterator_base(const std::uint8_t* m, Iter p, Iter end, bool /* skip_test */)
        : _meta_ptr(m), _ptr(p), _end(end) {}

    template <typename Other, typename OtherIter>
    dict_iterator_base(const dict_iterator_base<Other, OtherIter>& other)
        : _meta_ptr(other._meta_ptr), _ptr(other._ptr), _end(other._end) {}

private:
    friend class boost::iterator_core_access;
    template <typename, typename>
    friend class dict_iterator_base;

    void increment() {
        do {
            ++_ptr;
            ++_meta_ptr;
        } while (_ptr != _end && *_meta_ptr == 0);
    }

    template <typename OtherValue, typename OtherIter>
    bool equal(const dict_iterator_base<OtherValue, OtherIter>& other) const {
        return this->_ptr == other._ptr;
    }

    value_type& dereference() const { return _ptr->kv.const_view; }


    const std::uint8_t* _meta_ptr;
    Iter _ptr;
    const Iter _end;
};

template <typename Key, typename Value>
using dict_iterator = dict_iterator_base<
    std::pair<const Key, Value>,
    typename std::vector<detail::dict_entry<Key, Value>>::iterator>;

template <typename Key, typename Value>
using const_dict_iterator = dict_iterator_base<
    const std::pair<const Key, Value>,
    typename std::vector<detail::dict_entry<Key, Value>>::const_iterator>;

} // detail

} // namespace io

#endif
