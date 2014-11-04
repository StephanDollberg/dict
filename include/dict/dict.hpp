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
#include "detail/key_value.hpp"
#include "detail/iterator.hpp"

namespace boost {

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
