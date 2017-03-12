#ifndef DICT_HPP
#define DICT_HPP

#include <vector>
#include <stdexcept>
#include <tuple>
#include <functional>

#include "detail/prime.hpp"
#include "detail/key_value.hpp"
#include "detail/iterator.hpp"

#include "detail/entry.hpp"

namespace io {

namespace detail {
    struct insert_result {
        std::size_t index;
        bool key_existed;
    };
}

// container
template <typename Key, typename Value, typename Hasher = std::hash<Key>,
          typename KeyEqual = std::equal_to<Key>,
          typename Allocator = std::allocator<std::pair<const Key, Value>>>
class dict {
public:
    typedef Key key_type;
    typedef Value mapped_type;
    typedef Allocator allocator_type;
    typedef Hasher hasher;
    typedef KeyEqual key_equal;

    typedef std::pair<const Key, Value> value_type;

private:
    typedef detail::dict_entry<Key, Value> entry_type;
    typedef typename std::allocator_traits<Allocator>::template rebind_alloc<
        entry_type> _internal_allocator;
    typedef std::vector<entry_type, _internal_allocator> table_type;

public:
    typedef typename table_type::size_type size_type;
    typedef typename table_type::difference_type difference_type;
    typedef value_type& reference;

    typedef detail::dict_iterator<Key, Value> iterator;
    typedef detail::const_dict_iterator<Key, Value> const_iterator;

    dict() : dict(initial_size()) {}

    explicit dict(size_type initial_size, const Hasher& hash = Hasher(),
                  const KeyEqual& key_equal = KeyEqual(),
                  const Allocator& alloc = Allocator())
        : _table(_internal_allocator(alloc)), _element_count(0),
          _max_element_count(initial_size), _hasher(hash),
          _key_equal(key_equal) {
        // we need to do this manually because only as of C++14 there is
        // explicit vector( size_type count, const Allocator& alloc =
        // Allocator() );
        _table.resize(next_size(initial_size, initial_load_factor()));
        _max_element_count = initial_load_factor() * _table.size();
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

    allocator_type get_allocator() const noexcept {
        return _table.get_allocator();
    }

    iterator begin() noexcept { return { _table.begin(), _table.end() }; }

    const_iterator begin() const noexcept {
        return { _table.begin(), _table.end() };
    }

    const_iterator cbegin() const noexcept {
        return { _table.begin(), _table.end() };
    }

    iterator end() noexcept { return { _table.end(), _table.end(), true }; }

    const_iterator end() const noexcept {
        return { _table.end(), _table.end(), true };
    }

    const_iterator cend() const noexcept {
        return { _table.end(), _table.end(), true };
    }

    size_type size() const noexcept { return _element_count; }

    bool empty() const noexcept { return _element_count == 0; }

    // size_type max_size() const {}

    void clear() {
        // this could optimized to not re-default init everything
        auto old_size = _table.size();
        _table.clear();
        _table.resize(old_size);
        _element_count = 0;
    }

    template <typename... Args>
    std::pair<iterator, bool> emplace(Args&&... args) {
        return insert_entry(std::forward<Args>(args)...);
    }

    template <typename... Args>
    iterator emplace_hint(const_iterator /* hint */, Args&&... args) {
        return emplace(std::forward<Args>(args)...).first;
    }

    template <typename... Args>
    std::pair<iterator, bool> try_emplace(const key_type& key, Args&&... args) {
        return insert_element(key, std::forward<Args>(args)...);
    }

    template <typename... Args>
    std::pair<iterator, bool> try_emplace(key_type&& key, Args&&... args) {
        return insert_element(std::move(key), std::forward<Args>(args)...);
    }

    template <typename... Args>
    iterator try_emplace(const_iterator /* hint */, const key_type& key,
                         Args&&... args) {
        return try_emplace(key, std::forward<Args>(args)...).first;
    }

    template <typename... Args>
    iterator try_emplace(const_iterator /* hint */, key_type&& key,
                         Args&&... args) {
        return try_emplace(std::move(key), std::forward<Args>(args)...).first;
    }

    std::pair<iterator, bool> insert(const value_type& obj) {
        return insert_element(obj.first, obj.second);
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

    template <typename Mapped>
    std::pair<iterator, bool> insert_or_assign(const key_type& key,
                                               Mapped&& mapped) {
        return insert_assign_element(key, std::forward<Mapped>(mapped));
    }

    template <typename Mapped>
    std::pair<iterator, bool> insert_or_assign(key_type&& key,
                                               Mapped&& mapped) {
        return insert_assign_element(std::move(key),
                                     std::forward<Mapped>(mapped));
    }

    template <typename M>
    iterator insert_or_assign(const_iterator /* hint */, const key_type& key,
                              M&& obj) {
        return insert_or_assign(key, std::forward<M>(obj)).first;
    }

    template <typename M>
    iterator insert_or_assign(const_iterator /* hint */, key_type&& key,
                              M&& obj) {
        return insert_or_assign(std::move(key), std::forward<M>(obj)).first;
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

        if (index != _table.size()) {
            return iterator_from_index(index);
        } else {
            return end();
        }
    }

    const_iterator find(const Key& key) const {
        auto index = find_index(key);

        if (index != _table.size()) {
            return iterator_from_index(index);
        } else {
            return end();
        }
    }

    Value& at(const Key& key) {
        auto index = find_index(key);

        if (index != _table.size()) {
            return _table[index].kv.view.second;
        }

        throw std::out_of_range("Key not in dict");
    }

    const Value& at(const Key& key) const {
        auto index = find_index(key);

        if (index != _table.size()) {
            return _table[index].kv.view.second;
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

        if (index != _table.size()) {
            return _table[index].kv.view.second;
        } else {
            auto res = insert_element(key, Value());
            return res.first->second;
        }
    }

    size_type erase(const key_type& key) { return erase_impl(key).first; }

    iterator erase(const_iterator pos) { return erase_impl(pos->first).second; }

    // we don't support this (yet?)
    // iterator erase(const_iterator first, const_iterator last) {}

    float load_factor() const { return _element_count / float(_table.size()); }

    float max_load_factor() const {
        return _max_element_count / float(_table.size());
    }

    void max_load_factor(float new_max_load_factor) {
        _max_element_count = std::ceil(new_max_load_factor * _table.size());

        // a load factor of 1 would make index finding never stop
        if (_max_element_count == _table.size()) {
            _max_element_count -= 1;
        }

        if (next_is_rehash()) {
            rehash();
        }
    }

    void reserve(std::size_t new_size) {
        if (new_size > _table.size()) {
            table_type new_table(_table.get_allocator());
            new_table.resize(next_size(new_size, max_load_factor()));

            for (auto&& e : _table) {
                if (e.used) {
                    insert_index_impl(std::move_if_noexcept(e.kv.view.first),
                                        std::move_if_noexcept(e.kv.view.second), new_table);
                }
            }

            _max_element_count = max_load_factor() * new_table.size();
            _table.swap(new_table);
        }
    }

    // new size doesn't really matter here as we enforce power of two in reserve
    // so we simply add one to get the next block
    void rehash() { reserve(_table.size() + 1); }

    bool next_is_rehash() const { return size() >= _max_element_count; }

    hasher hash_function() const { return _hasher; }

    key_equal key_eq() const { return _key_equal; }

private:
    void check_expand() {
        if (next_is_rehash()) {
            rehash();
        }
    }

    // insert by variadic arg pack (including key)
    template <typename... Args>
    std::pair<iterator, bool> insert_entry(Args&&... args) {
        check_expand();
        auto new_entry = make_entry(std::forward<Args>(args)...);
        auto res = insert_index(std::move(new_entry.kv.view.first),
                                std::move(new_entry.kv.view.second));
        auto index = res.index;

        if (res.key_existed) {
            return { iterator_from_index(index), false };
        } else {
            ++_element_count;
            return { iterator_from_index(index), true };
        }
    }

    // insert by key + mapped value
    template <typename KeyParam, typename Mapped>
    std::pair<iterator, bool> insert_element(KeyParam&& key, Mapped&& mapped) {
        check_expand();
        auto res = insert_index(std::forward<KeyParam>(key),
                                std::forward<Mapped>(mapped));
        auto index = res.index;

        if (res.key_existed) {
            return { iterator_from_index(index), false };
        } else {
            ++_element_count;
            return { iterator_from_index(index), true };
        }
    }

    // insert or overwrite by key + mapped value
    template <typename KeyParam, typename Mapped>
    std::pair<iterator, bool> insert_assign_element(KeyParam&& key,
                                                    Mapped&& mapped) {
        check_expand();
        auto res = insert_index(std::forward<KeyParam>(key),
                                std::forward<Mapped>(mapped));
        auto index = res.index;

        if (res.key_existed) {
            _table[index].kv.view.second = std::move(mapped);
            return { iterator_from_index(index), false };
        } else {
            ++_element_count;
            return { iterator_from_index(index), true };
        }
    }

    std::pair<size_type, iterator> erase_impl(const Key& key) {
        auto index = find_index(key);

        if (index == _table.size()) {
            return { 0, {} };
        }

        const auto deleted_index = index;

        _table[index] = empty_slot_factory();
        --_element_count;

        auto delete_index = index;
        while (true) {
            delete_index = next_index(delete_index);

            if (!_table[delete_index].used) {
                return { 1,
                         { std::next(_table.begin(), deleted_index),
                           _table.end() } };
            }

            auto new_key = hash_index(_table[delete_index].kv.view.first);

            if ((index <= delete_index)
                    ? ((index < new_key) && (new_key <= delete_index))
                    : ((index < new_key) || (new_key <= delete_index))) {
                continue;
            }

            // swapping previously emptied index with delete_index
            using std::swap;
            swap(_table[index], _table[delete_index]);
            index = delete_index;
        }
    }

    size_type find_index(const Key& key) const {
        return find_index_impl(key, _table);
    }

    // returns table.size() if element is not found
    size_type find_index_impl(const Key& key, const table_type& table) const {
        const auto key_hash = _hasher(key);
        auto index = map_position_impl(key_hash, table);
        auto distance = 0u;

        while (true) {
            if (!table[index].used) {
                return table.size();
            } else if (distance >
                       probe_distance(table[index].hash, index)) {
                return table.size();
            } else if (table[index].hash == key_hash &&
                       table[index].kv.const_view.first == key) {
                return index;
            }

            index = next_index_impl(index, table);
            ++distance;
        }
    }

    template <typename KeyParam, typename Mapped>
    detail::insert_result insert_index(KeyParam&& key, Mapped&& mapped) {
        return insert_index_impl(std::forward<KeyParam>(key),
                                 std::forward<Mapped>(mapped), _table);
    }

    template <typename KeyParam, typename Mapped>
    detail::insert_result insert_index_impl(KeyParam&& key,
                                                 Mapped&& mapped,
                                                 table_type& table) const {
        auto key_hash = _hasher(key);
        auto index = map_position_impl(key_hash, table);
        size_type distance = 0u;

        while (true) {
            if (!table[index].used) {
                table[index] = make_entry(std::forward<KeyParam>(key),
                                           std::forward<Mapped>(mapped));
                table[index].hash = key_hash;

                return { index, false };
            }

            if (table[index].hash == key_hash &&
                table[index].kv.view.first == key) {
                return { index, true };
            }

            auto current_element_distance =
                probe_distance_impl(table[index].hash, index, table);
            if (current_element_distance < distance) {
                Key k = std::move(key);
                Value v = std::move(mapped);
                using std::swap;
                swap(table[index].kv.view.first, k);
                swap(table[index].kv.view.second, v);
                swap(table[index].hash, key_hash);

                insert_index_impl_tail(k, v, current_element_distance, key_hash,
                                       index, table);
                return { index, false };
            }

            index = next_index_impl(index, table);
            ++distance;
        }
    }

    template <typename KeyParam, typename Mapped>
    void insert_index_impl_tail(KeyParam& key, Mapped& mapped,
                                size_type distance, size_type key_hash,
                                size_type index, table_type& table) const {

        index = next_index_impl(index, table);
        ++distance;

        while (true) {
            if (!table[index].used) {
                table[index] = make_entry(std::forward<KeyParam>(key),
                                          std::forward<Mapped>(mapped));
                table[index].hash = key_hash;
                return;
            }

            auto current_element_distance =
                probe_distance_impl(table[index].hash, index, table);
            if (current_element_distance < distance) {
                using std::swap;
                swap(table[index].kv.view.first, key);
                swap(table[index].kv.view.second, mapped);
                swap(table[index].hash, key_hash);
                distance = current_element_distance;
            }

            index = next_index_impl(index, table);
            ++distance;
        }
    }

    size_type probe_distance(size_type hash, size_type index) const {
        return probe_distance_impl(hash, index, _table);
    }

    size_type probe_distance_impl(size_type hash, size_type index, const table_type& table) const {
        auto root_index = map_position_impl(hash, table);
        return map_position_impl(index + table.size() - root_index, table);
    }

    size_type hash_index(const Key& key) const {
        return hash_index_impl(key, _table);
    }

    size_type hash_index_impl(const Key& key, const table_type& table) const {
        return map_position_impl(_hasher(key), table);
    }

    size_type map_position(size_type hash) const {
        return map_position_impl(hash, _table);
    }

    size_type map_position_impl(size_type hash, const table_type& table) const {
        return hash & (table.size() - 1);
    }

    constexpr size_type next_index(size_type index) const {
        return next_index_impl(index, _table);
    }

    constexpr size_type next_index_impl(size_type index,
                                        const table_type& table) const {
        return (index + 1) & (table.size() - 1);
    }

    template <typename... Args>
    entry_type make_entry(Args&&... args) const {
        return {
            detail::key_value<Key, Value>(std::forward<Args>(args)...), 0, true};
    }

    entry_type empty_slot_factory() const {
        return {
            detail::key_value<Key, Value>(Key(), Value()), 0, false};
    }

    iterator iterator_from_index(size_type index) {
        return { std::next(_table.begin(), index), _table.end(), true };
    }

    const_iterator iterator_from_index(size_type index) const {
        return { std::next(_table.begin(), index), _table.end(), true };
    }

    size_type initial_size() const { return detail::next_power_of_two(8); }

    constexpr size_type next_size(size_type min_size,
                                  double load_factor) const {
        return detail::next_power_of_two(std::ceil(min_size / load_factor));
    }

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

template <typename Key, typename Value, typename Hasher, typename KeyEqual,
          typename Allocator>
bool operator==(dict<Key, Value, Hasher, KeyEqual, Allocator>& A,
                dict<Key, Value, Hasher, KeyEqual, Allocator>& B) {
    if (A.size() != B.size()) {
        return false;
    }

    for (const auto& elem : A) {
        auto res = B.find(elem.first);

        if (res == B.end() || res->second != elem.second) {
            return false;
        }
    }

    return true;
}

template <typename Key, typename Value, typename Hasher, typename KeyEqual,
          typename Allocator>
bool operator!=(dict<Key, Value, Hasher, KeyEqual, Allocator>& A,
                dict<Key, Value, Hasher, KeyEqual, Allocator>& B) {
    return !(A == B);
}

} // namespace io

#endif
