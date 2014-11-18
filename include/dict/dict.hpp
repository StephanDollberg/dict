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

    dict() : dict(initial_size()) {}

    explicit dict(size_type initial_size, const Hasher& hash = Hasher(),
                  const KeyEqual& key_equal = KeyEqual(),
                  const Allocator& alloc = Allocator())
        : _table(alloc), _element_count(0), _max_element_count(initial_size),
          _hasher(hash), _key_equal(key_equal) {
        // we need to do this manually because only as of C++14 there is
        // explicit vector( size_type count, const Allocator& alloc =
        // Allocator() );
        _table.resize(
            next_prime(std::ceil(initial_size / initial_load_factor())));
    }

    explicit dict(const Allocator& alloc)
        : dict(initial_size(), Hasher(), KeyEqual(), alloc) {}

    template <typename Iter>
    dict(Iter begin, Iter end, size_type /* dummy_initial_size */ = 0,
         const Hasher& hash = Hasher(), const KeyEqual& key_equal = KeyEqual(),
         const Allocator& alloc = Allocator())
        : dict(initial_size(), hash, key_equal, alloc) {
        for (auto iter = begin; iter != end; ++iter) {
            emplace(iter->first, iter->second);
        }
    }

    template <typename Iter>
    dict(Iter begin, Iter end, size_type initial_size, const Allocator& alloc)
        : dict(begin, end, initial_size, Hasher(), KeyEqual(), alloc) {}

    template <typename Iter>
    dict(Iter begin, Iter end, size_type initial_size, const Hasher& hasher,
         const Allocator& alloc)
        : dict(begin, end, initial_size, hasher, KeyEqual(), alloc) {}

    dict(std::initializer_list<value_type> init,
         size_type /* dummy_initial_size */ = 0, const Hasher& hash = Hasher(),
         const KeyEqual& key_equal = KeyEqual(),
         const Allocator& alloc = Allocator())
        : dict(init.begin(), init.end(), initial_size(), hash, key_equal,
               alloc) {}

    dict(std::initializer_list<value_type> init, size_type initial_size,
         const Allocator& alloc)
        : dict(init.begin(), init.end(), initial_size(), Hasher(), KeyEqual(),
               alloc) {}

    dict(std::initializer_list<value_type> init, size_type initial_size,
         const Hasher& hasher, const Allocator& alloc)
        : dict(init.begin(), init.end(), initial_size(), hasher, KeyEqual(),
               alloc) {}

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
    iterator emplace_hint(const_iterator /* hint */, Args&&... args) {
        return emplace(std::forward<Args>(args)...).first;
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

    template <typename P,
              typename = typename std::enable_if<
                  std::is_constructible<value_type, P&&>::value>::type>
    std::pair<iterator, bool> insert(P&& value) {
        return emplace(std::forward<P>(value));
    }

    std::pair<iterator, bool> insert(const_iterator /* hint */,
                                     const value_type& obj) {
        return insert(obj);
    }

    template <typename P,
              typename = typename std::enable_if<
                  std::is_constructible<value_type, P&&>::value>::type>
    std::pair<iterator, bool> insert(const_iterator hint, P&& value) {
        return emplace_hint(hint, std::forward<P>(value));
    }

    template <typename InputIt>
    void insert(InputIt begin, InputIt end) {
        for (auto iter = begin; iter != end; ++iter) {
            emplace(iter->first, iter->second);
        }
    }

    void insert(std::initializer_list<value_type> init) {
        insert(init.begin(), init.end());
    }

    void swap(dict<Key, Value, Hasher, KeyEqual, Allocator>& other) {
        using std::swap;
        swap(_table, other._table);
        swap(_element_count, other._element_count);
        swap(_max_element_count, other._max_element_count);
        swap(_key_equal, other._key_equal);
        swap(_hasher, other._hasher);
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

    table_type _table;
    size_type _element_count;
    size_type _max_element_count;
    hasher _hasher;
    key_equal _key_equal;
};

template <typename Key, typename Value, typename Hasher, typename KeyEqual,
          typename Allocator>
void swap(dict<Key, Value, Hasher, KeyEqual, Allocator>& A,
          dict<Key, Value, Hasher, KeyEqual, Allocator>& B) {
    A.swap(B);
}

} // namespace boost

#endif
