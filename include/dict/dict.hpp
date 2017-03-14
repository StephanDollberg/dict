#ifndef DICT_HPP
#define DICT_HPP

#include <vector>
#include <stdexcept>
#include <tuple>
#include <functional>
#include <limits>

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
    typedef std::vector<typename table_type::size_type, _internal_allocator> indecies_table_type;

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
        : _indices(_internal_allocator(alloc)), _entries(_internal_allocator(alloc)), _element_count(0),
          _max_element_count(initial_size), _hasher(hash),
          _key_equal(key_equal) {
        // we need to do this manually because only as of C++14 there is
        // explicit vector( size_type count, const Allocator& alloc =
        // Allocator() );
        _indices.resize(next_size(initial_size, initial_load_factor()), free_index());
        _max_element_count = initial_load_factor() * _indices.size();
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
        return _entries.get_allocator();
    }

    iterator begin() noexcept { return { _entries.begin(), _entries.end() }; }

    const_iterator begin() const noexcept {
        return { _entries.begin(), _entries.end() };
    }

    const_iterator cbegin() const noexcept {
        return { _entries.begin(), _entries.end() };
    }

    iterator end() noexcept { return { _entries.end(), _entries.end(), true }; }

    const_iterator end() const noexcept {
        return { _entries.end(), _entries.end(), true };
    }

    const_iterator cend() const noexcept {
        return { _entries.end(), _entries.end(), true };
    }

    size_type size() const noexcept { return _element_count; }

    bool empty() const noexcept { return _element_count == 0; }

    // size_type max_size() const {}

    void clear() {
        // this could optimized to not re-default init everything
        auto old_size = _entries.size();
        _entries.clear();
        _entries.reserve(old_size);

        old_size = _indices.size();
        _indices.clear();
        _indices.resize(old_size, free_index());

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
        swap(_indices, other._indices);
        swap(_entries, other._entries);
        swap(_element_count, other._element_count);
        swap(_max_element_count, other._max_element_count);
        swap(_key_equal, other._key_equal);
        swap(_hasher, other._hasher);
    }

    iterator find(const Key& key) {
        auto index = find_index(key);

        if (index != _entries.size()) {
            return iterator_from_index(index);
        } else {
            return end();
        }
    }

    const_iterator find(const Key& key) const {
        auto index = find_index(key);

        if (index != _entries.size()) {
            return iterator_from_index(index);
        } else {
            return end();
        }
    }

    Value& at(const Key& key) {
        auto index = find_index(key);

        if (index != _entries.size()) {
            return _entries[index].kv.view.second;
        }

        throw std::out_of_range("Key not in dict");
    }

    const Value& at(const Key& key) const {
        auto index = find_index(key);

        if (index != _entries.size()) {
            return _entries[index].kv.view.second;
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

        if (index != _entries.size()) {
            return _entries[index].kv.view.second;
        } else {
            auto res = insert_element(key, Value());
            return res.first->second;
        }
    }

    size_type erase(const key_type& key) { return erase_impl(key).first; }

    iterator erase(const_iterator pos) { return erase_impl(pos->first).second; }

    // we don't support this (yet?)
    // iterator erase(const_iterator first, const_iterator last) {}

    float load_factor() const { return _element_count / float(_indices.size()); }

    float max_load_factor() const {
        return _max_element_count / float(_indices.size());
    }

    void max_load_factor(float new_max_load_factor) {
        _max_element_count = std::ceil(new_max_load_factor * _indices.size());

        // a load factor of 1 would make index finding never stop
        if (_max_element_count == _indices.size()) {
            _max_element_count -= 1;
        }

        if (next_is_rehash()) {
            rehash();
        }
    }

    void reserve(std::size_t new_size) {
        if (new_size > _indices.size()) {
            indecies_table_type new_indices(_indices.get_allocator());
            new_indices.resize(next_size(new_size, max_load_factor()), free_index());

            for (auto mapped_index : _indices) {
                if (mapped_index != free_index()) {
                    auto key_hash = _entries[mapped_index].hash;
                    auto new_index = map_position_impl(key_hash, new_indices);
                    auto new_mapped_index = new_indices[new_index];
                    size_type distance = 0u;

                    while (true) {
                        if (new_mapped_index == free_index()) {
                            new_indices[new_index] = mapped_index;

                            break;
                        }

                        auto current_element_distance =
                            probe_distance_impl(_entries[new_mapped_index].hash, new_index, new_indices);
                        if (current_element_distance < distance) {
                            new_indices[new_index] = mapped_index;

                            insert_index_impl_tail(new_mapped_index, current_element_distance,
                                                   new_index, new_indices, _entries);
                            break;
                        }

                        new_index = next_index_impl(new_index, new_indices);
                        new_mapped_index = new_indices[new_index];
                        ++distance;
                    }
                }

            }

            _max_element_count = max_load_factor() * new_indices.size();
            _indices.swap(new_indices);
        }
    }

    // new size doesn't really matter here as we enforce power of two in reserve
    // so we simply add one to get the next block
    void rehash() { reserve(_indices.size() + 1); }

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
            _entries[index].kv.view.second = std::move(mapped);
            return { iterator_from_index(index), false };
        } else {
            ++_element_count;
            return { iterator_from_index(index), true };
        }
    }

    std::pair<size_type, iterator> erase_impl(const Key& key) {
        auto index = find_index(key);

        if (index == free_index()) {
            return { 0, {} };
        }

        // TODO impl this

        return { 0, {} };

        // const auto deleted_index = index;

        // _table[index] = empty_slot_factory();
        // --_element_count;

        // auto delete_index = index;
        // while (true) {
        //     delete_index = next_index(delete_index);

        //     if (_table[delete_index].entry_is_free()) {
        //         return { 1,
        //                  { std::next(_table.begin(), deleted_index),
        //                    _table.end() } };
        //     }

        //     auto new_key = hash_index(_table[delete_index].kv.view.first);

        //     if ((index <= delete_index)
        //             ? ((index < new_key) && (new_key <= delete_index))
        //             : ((index < new_key) || (new_key <= delete_index))) {
        //         continue;
        //     }

        //     // swapping previously emptied index with delete_index
        //     using std::swap;
        //     swap(_table[index], _table[delete_index]);
        //     index = delete_index;
        // }
    }

    size_type find_index(const Key& key) const {
        return find_index_impl(key, _indices, _entries);
    }

    // returns table.size() if element is not found
    size_type find_index_impl(const Key& key, const indecies_table_type& indices, const table_type& entries) const {
        const auto key_hash = safe_hash(key);
        auto index = map_position_impl(key_hash, indices);
        auto mapped_index = indices[index];
        auto distance = 0u;

        while (true) {
            if (mapped_index == free_index()) {
                return entries.size();
            } else if (distance >
                       probe_distance(entries[mapped_index].hash, index)) {
                return entries.size();
            } else if (entries[mapped_index].hash == key_hash &&
                       entries[mapped_index].kv.const_view.first == key) {
                return mapped_index;
            }

            index = next_index_impl(index, indices);
            mapped_index = indices[index];
            ++distance;
        }
    }

    template <typename KeyParam, typename Mapped>
    detail::insert_result insert_index(KeyParam&& key, Mapped&& mapped) {
        return insert_index_impl(std::forward<KeyParam>(key),
                                 std::forward<Mapped>(mapped), _indices, _entries);
    }

    template <typename KeyParam, typename Mapped>
    detail::insert_result insert_index_impl(KeyParam&& key,
                                                 Mapped&& mapped,
                                                 indecies_table_type& indices,
                                                 table_type& entries) const {
        auto key_hash = safe_hash(key);
        auto index = map_position_impl(key_hash, indices);
        auto mapped_index = indices[index];
        size_type distance = 0u;

        while (true) {
            if (mapped_index == free_index()) {
                entries.emplace_back(make_entry(std::forward<KeyParam>(key),
                                           std::forward<Mapped>(mapped)));
                entries.back().hash = key_hash;

                mapped_index = entries.size() - 1;
                indices[index] = mapped_index;

                return { mapped_index, false };
            }

            if (entries[mapped_index].hash == key_hash &&
                entries[mapped_index].kv.view.first == key) {
                return { mapped_index, true };
            }

            auto current_element_distance =
                probe_distance_impl(entries[mapped_index].hash, index, indices);
            if (current_element_distance < distance) {
                entries.emplace_back(make_entry(std::forward<KeyParam>(key),
                                           std::forward<Mapped>(mapped)));
                entries.back().hash = key_hash;

                mapped_index = entries.size() - 1;

                auto last_mapped_index = indices[index];
                indices[index] = mapped_index;

                insert_index_impl_tail(last_mapped_index, current_element_distance,
                                       index, indices, entries);
                return { mapped_index, false };
            }

            index = next_index_impl(index, indices);
            mapped_index = indices[index];
            ++distance;
        }
    }

    void insert_index_impl_tail(size_type last_mapped_index, size_type distance,
                                size_type index, indecies_table_type& indices, table_type& entries) const {

        index = next_index_impl(index, indices);
        auto mapped_index = indices[index];
        ++distance;

        while (true) {
            if (mapped_index == free_index()) {
                indices[index] = last_mapped_index;
                return;
            }

            auto current_element_distance =
                probe_distance_impl(entries[mapped_index].hash, index, indices);
            if (current_element_distance < distance) {
                indices[index] = last_mapped_index;
                last_mapped_index = mapped_index;
                distance = current_element_distance;
            }

            index = next_index_impl(index, indices);
            mapped_index = indices[index];
            ++distance;
        }
    }

    size_type probe_distance(size_type hash, size_type index) const {
        return probe_distance_impl(hash, index, _indices);
    }

    size_type probe_distance_impl(size_type hash, size_type index, const indecies_table_type& table) const {
        auto root_index = map_position_impl(hash, table);
        return map_position_impl(index + table.size() - root_index, table);
    }

    size_type hash_index(const Key& key) const {
        return hash_index_impl(key, _indices);
    }

    size_type hash_index_impl(const Key& key, const indecies_table_type& table) const {
        return map_position_impl(safe_hash(key), table);
    }

    size_type map_position(size_type hash) const {
        return map_position_impl(hash, _indices);
    }

    size_type map_position_impl(size_type hash, const indecies_table_type& table) const {
        return hash & (table.size() - 1);
    }

    constexpr size_type next_index(size_type index) const {
        return next_index_impl(index, _indices);
    }

    constexpr size_type next_index_impl(size_type index,
                                        const indecies_table_type& table) const {
        return (index + 1) & (table.size() - 1);
    }

    template <typename... Args>
    entry_type make_entry(Args&&... args) const {
        return {
            detail::key_value<Key, Value>(std::forward<Args>(args)...), 0};
    }

    entry_type empty_slot_factory() const {
        return {
            detail::key_value<Key, Value>(Key(), Value()), 0};
    }

    iterator iterator_from_index(size_type index) {
        return { std::next(_entries.begin(), index), _entries.end(), true };
    }

    const_iterator iterator_from_index(size_type index) const {
        return { std::next(_entries.begin(), index), _entries.end(), true };
    }

    size_type initial_size() const { return detail::next_power_of_two(8); }

    constexpr size_type next_size(size_type min_size,
                                  double load_factor) const {
        return detail::next_power_of_two(std::ceil(min_size / load_factor));
    }

    constexpr float initial_load_factor() const { return 0.7; }

    // inspired by Rust HashMap
    // we set MSB to one as 0 is special value to indicate empty element
    constexpr size_type safe_hash(const Key& key) const {
        auto hash = _hasher(key);
        auto hash_bits = sizeof(size_type) * 8;
        return (std::size_t(1) << (hash_bits - 1)) | hash;
    }

    constexpr size_type free_index() const {
        return std::numeric_limits<size_type>::max();
    }

    indecies_table_type _indices;
    table_type _entries;
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
