#ifndef DICT_HPP
#define DICT_HPP

#include <string>
#include <vector>
#include <stdexcept>
#include <tuple>
#include <cmath>
#include <functional>

#include <boost/iterator/iterator_facade.hpp>

#include "detail/prime.hpp"

namespace boost {

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

    key_value(key_value&& other) : view(std::move(other.view)) {}

    key_value& operator=(const key_value& other) {
        view = other.const_view;
        return *this;
    }

    key_value& operator=(key_value&& other) {
        view = std::move(other.view);
        return *this;
    }

    ~key_value() { const_view.~value_type(); }
};

// iterators
template <typename value_type, typename Iter>
class dict_iterator_base
    : public boost::iterator_facade<dict_iterator_base<value_type, Iter>,
                                    value_type, boost::forward_traversal_tag> {
public:
    dict_iterator_base() : _ptr(), _end() {}
    dict_iterator_base(Iter p, Iter end) : _ptr(p), _end(end) {
        if (!std::get<0>(*_ptr)) {
            increment();
        }
    }

private:
    friend class boost::iterator_core_access;

    void increment() {
        while (_ptr != _end) {
            ++_ptr;
            if (std::get<0>(*_ptr)) {
                break;
            }
        }
    }

    // template<typename OtherValue>
    bool equal(const dict_iterator_base<value_type, Iter>& other) const {
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
// container
template <typename Key, typename Value, typename Hasher = std::hash<Key>,
          typename KeyEqual = std::equal_to<Key>,
          typename Allocator =
              std::allocator<std::tuple<bool, key_value<Key, Value>>>>
class dict {
public:
    typedef Key key_type;
    typedef Value mapped_type;
    typedef Allocator allocator_type;
    typedef Hasher hasher;
    typedef KeyEqual key_equal;

    typedef std::pair<const Key, Value> value_type;
    typedef std::tuple<bool, key_value<Key, Value>> entry_type;
    typedef std::vector<entry_type, allocator_type> table_type;
    typedef typename table_type::size_type size_type;
    typedef typename table_type::difference_type difference_type;
    typedef value_type& reference;

    typedef dict_iterator<Key, Value> iterator;
    typedef const_dict_iterator<Key, Value> const_iterator;

    dict()
        : _table(initial_size(), empty_slot_factory()), _element_count(0),
          _max_element_count(load_factor() * _table.size()) {}

    iterator begin() noexcept {
        return { _table.begin(), _table.end() };
    }

    const_iterator begin() const noexcept {
        return { _table.begin(), _table.end() };
    }

    const_iterator cbegin() const noexcept {
        return { _table.begin(), _table.end() };
    }

    iterator end() noexcept {
        return { _table.end(), _table.end() };
    }

    const_iterator end() const noexcept {
        return { _table.end(), _table.end() };
    }

    const_iterator cend() const noexcept {
        return { _table.end(), _table.end() };
    }

    size_type size() const noexcept { return _element_count; }

    bool empty() const noexcept { return _element_count == 0; }

    // size_type max_size() const {}

    void clear() {
        _table.clear();
        _element_count = 0;
    }

    std::pair<iterator, bool> insert(const value_type& obj) {
        auto index = find_index(obj.first);

        if (std::get<0>(_table[index])) {
            return { { std::next(_table.begin(), index), _table.end() },
                     false };
        } else {
            insert_element(index, obj.first, obj.second);
            return { { std::next(_table.begin(), index), _table.end() }, true };
        }
    }

    Value& operator[](const Key& key) {
        auto index = find_index(key);

        if (std::get<0>(_table[index])) {
            return std::get<1>(_table[index]).view.second;
        } else {
            return insert_element(index, key, Value());
        }
    }

    size_type erase(const Key& key) {
        auto index = find_index(key);

        if (!std::get<0>(_table[index])) {
            return 0;
        }

        auto delete_index = index;

        bool keep = true;
        while (keep) {
            _table[index] = empty_slot_factory();

            while (true) {
                delete_index = (delete_index + 1) % _table.size();

                if (!std::get<0>(_table[delete_index])) {
                    keep = false;
                    break;
                }

                auto new_key =
                    hash_index(std::get<1>(_table[delete_index]).view.first);

                if ((index <= delete_index)
                        ? ((index < new_key) && (new_key <= delete_index))
                        : (new_key <= delete_index)) {
                    continue;
                }

                _table[index] = std::move(_table[delete_index]);
                index = delete_index;
            }
        }

        return 1;
    }

    void reserve(std::size_t new_size) {
        if (new_size > _table.size()) {

            table_type new_table(
                next_prime(std::ceil(new_size / load_factor())),
                empty_slot_factory());
            new_table.swap(_table);
            _max_element_count = load_factor() * _table.size();
            _element_count = 0;

            for (auto&& e : new_table) {
                if (std::get<0>(e)) {
                    (*this)[std::get<1>(e).view.first] =
                        std::move(std::get<1>(e).view.second);
                }
            }
        }
    }

private:
    Value& insert_element(size_type index, Key key, Value value) {
        rehash();

        _table[index] = std::make_tuple(
            true, std::make_pair(std::move(key), std::move(value)));
        ++_element_count;

        return std::get<1>(_table[index]).view.second;
    }

    size_type find_index(const Key& key) {
        auto index = hash_index(key);

        while (std::get<0>(_table[index])) {
            if (_key_equal(std::get<1>(_table[index]).view.first, key)) {
                return index;
            }

            index = (index + 1) % _table.size();
        }

        return index;
    }

    void rehash() {
        if (size() > _max_element_count) {
            reserve(2 * _table.size());
        }
    }

    double load_factor() const { return 0.7; }

    size_type hash_index(const Key& key) {
        return _hasher(key) % _table.size();
    }

    entry_type empty_slot_factory() const {
        return std::make_tuple(false, std::make_pair(Key(), Value()));
    }

    size_type initial_size() const { return next_prime(8); }

    hasher _hasher;
    key_equal _key_equal;
    table_type _table;
    size_type _element_count;
    size_type _max_element_count;
};

} // namespace boost

#endif
