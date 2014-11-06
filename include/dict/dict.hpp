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
              std::allocator<std::tuple<bool, detail::key_value<Key, Value>>>>
class dict {
public:
    typedef Key key_type;
    typedef Value mapped_type;
    typedef Allocator allocator_type;
    typedef Hasher hasher;
    typedef KeyEqual key_equal;

    typedef std::pair<const Key, Value> value_type;
    typedef std::tuple<bool, detail::key_value<Key, Value>> entry_type;
    typedef std::vector<entry_type, allocator_type> table_type;
    typedef typename table_type::size_type size_type;
    typedef typename table_type::difference_type difference_type;
    typedef value_type& reference;

    typedef detail::dict_iterator<Key, Value> iterator;
    typedef detail::const_dict_iterator<Key, Value> const_iterator;

    dict()
        : _table(initial_size()), _element_count(0),
          _max_element_count(initial_load_factor() * _table.size()) {}

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

    template <typename... Args>
    std::pair<iterator, bool> emplace(Args&&... args) {
        auto entry = make_entry(std::forward<Args>(args)...);
        auto index = find_index(std::get<1>(entry).view.first);

        if (std::get<0>(_table[index])) {
            return { { std::next(_table.begin(), index), _table.end() },
                     false };
        } else {
            insert_element(index, std::move(entry));
            return { { std::next(_table.begin(), index), _table.end() }, true };
        }
    }

    template <typename... Args>
    std::pair<iterator, bool> emplace_hint(const_iterator /* hint */,
                                           Args&&... args) {
        return emplace(std::forward<Args>(args)...);
    }

    std::pair<iterator, bool> insert(const value_type& obj) {
        auto index = find_index(obj.first);

        if (std::get<0>(_table[index])) {
            return { { std::next(_table.begin(), index), _table.end() },
                     false };
        } else {
            insert_element(index, make_entry(obj.first, obj.second));
            return { { std::next(_table.begin(), index), _table.end() }, true };
        }
    }

    iterator find(const Key& key) {
        auto index = find_index(key);

        if (std::get<0>(_table[index])) {
            return { std::next(_table.begin(), index), _table.end() };
        } else {
            return end();
        }
    }

    const_iterator find(const Key& key) const {
        auto index = find_index(key);

        if (std::get<0>(_table[index])) {
            return { std::next(_table.begin(), index), _table.end() };
        } else {
            return end();
        }
    }

    Value& at(const Key& key) {
        auto index = find_index(key);

        if (std::get<0>(_table[index])) {
            return std::get<1>(_table[index]).view.second;
        }

        throw std::out_of_range("Key not in dict");
    }

    const Value& at(const Key& key) const {
        auto index = find_index(key);

        if (std::get<0>(_table[index])) {
            return std::get<1>(_table[index]).view.second;
        }

        throw std::out_of_range("Key not in dict");
    }

    size_type count(const Key& key) const { return find(key) == end() ? 0 : 1; }

    std::pair<iterator, iterator> equal_range(const Key& key) {
        return { find(key), end() };
    }

    std::pair<const_iterator, const_iterator>
    equal_range(const Key& key) const {
        return { find(key), end() };
    }

    Value& operator[](const Key& key) {
        auto index = find_index(key);

        if (std::get<0>(_table[index])) {
            return std::get<1>(_table[index]).view.second;
        } else {
            return insert_element(index, make_entry(key, Value()));
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

    float load_factor() const { return _element_count / float(_table.size()); }

    float max_load_factor() const {
        return _max_element_count / float(_table.size());
    }

    void max_load_factor(float new_max_load_factor) {
        _max_element_count = std::ceil(new_max_load_factor * _table.size());
        if (check_rehash()) {
            rehash();
        }
    }

    void reserve(std::size_t new_size) {
        if (new_size > _table.size()) {
            auto old_load_factor = max_load_factor();

            table_type new_table(
                next_prime(std::ceil(new_size / old_load_factor)));
            new_table.swap(_table);
            _max_element_count = old_load_factor * _table.size();
            _element_count = 0;

            for (auto&& e : new_table) {
                if (std::get<0>(e)) {
                    (*this)[std::get<1>(e).view.first] =
                        std::move(std::get<1>(e).view.second);
                }
            }
        }
    }

    void rehash() { reserve(2 * _table.size()); }

    bool next_is_rehash() const { return size() + 1 >= _max_element_count; }

    hasher hash_function() const { return _hasher; }

    key_equal key_eq() const { return _key_equal; }

private:
    template <typename Entry>
    Value& insert_element(size_type index, Entry&& new_entry) {
        _table[index] = std::forward<Entry>(new_entry);
        ++_element_count;

        if (check_rehash()) {
            rehash();
            index = find_index(std::get<1>(new_entry).view.first);
        }

        return std::get<1>(_table[index]).view.second;
    }

    size_type find_index(const Key& key) const {
        auto index = hash_index(key);

        while (std::get<0>(_table[index])) {
            if (_key_equal(std::get<1>(_table[index]).view.first, key)) {
                return index;
            }

            index = (index + 1) % _table.size();
        }

        return index;
    }

    bool check_rehash() const { return size() >= _max_element_count; }

    size_type hash_index(const Key& key) const {
        return _hasher(key) % _table.size();
    }

    template <typename... Args>
    entry_type make_entry(Args&&... args) const {
        return std::make_tuple(
            true, detail::key_value<Key, Value>(std::forward<Args>(args)...));
    }

    entry_type empty_slot_factory() const {
        return std::make_tuple(false,
                               detail::key_value<Key, Value>(Key(), Value()));
    }

    size_type initial_size() const { return next_prime(8); }

    constexpr float initial_load_factor() const { return 0.7; }

    hasher _hasher;
    key_equal _key_equal;
    table_type _table;
    size_type _element_count;
    size_type _max_element_count;
};

} // namespace boost

#endif
