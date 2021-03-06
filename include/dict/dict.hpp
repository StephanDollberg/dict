#ifndef DICT_HPP
#define DICT_HPP

#include <functional>
#include <stdexcept>
#include <tuple>
#include <vector>
#if __cpp_lib_memory_resource
#include <experimental/memory_resource>
#endif

#include "detail/entry.hpp"
#include "detail/iterator.hpp"
#include "detail/key_value.hpp"
#include "detail/math_util.hpp"

namespace io {

// container
template <typename Key, typename Value, typename Hasher = std::hash<Key>,
          typename KeyEqual = std::equal_to<Key>,
          typename Allocator = std::allocator<std::pair<const Key, Value>>>
class dict {
public:
    using key_type = Key;
    using mapped_type = Value;
    using allocator_type = Allocator;
    using hasher = Hasher;
    using key_equal = KeyEqual;

    using value_type = std::pair<const Key, Value>;

private:
    using entry_type = detail::dict_entry<Key, Value>;
    using _internal_allocator =
        typename std::allocator_traits<Allocator>::template rebind_alloc<
        entry_type>;
    using table_type = std::vector<entry_type, _internal_allocator>;

public:
    using size_type = typename table_type::size_type;
    using difference_type = typename table_type::difference_type;
    using reference = value_type&;

    using iterator = detail::dict_iterator<Key, Value>;
    using const_iterator = detail::const_dict_iterator<Key, Value>;

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

        if (_table[index].used) {
            return iterator_from_index(index);
        } else {
            return end();
        }
    }

    const_iterator find(const Key& key) const {
        auto index = find_index(key);

        if (_table[index].used) {
            return iterator_from_index(index);
        } else {
            return end();
        }
    }

    Value& at(const Key& key) {
        auto index = find_index(key);

        if (_table[index].used) {
            return _table[index].kv.view.second;
        }

        throw std::out_of_range("Key not in dict");
    }

    const Value& at(const Key& key) const {
        auto index = find_index(key);

        if (_table[index].used) {
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

        if (_table[index].used) {
            return _table[index].kv.view.second;
        } else {
            check_expand();
            return activate_element(find_index(key), key);
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
                    auto new_index =
                        find_index_impl(e.kv.view.first, new_table);
                    new_table[new_index] = std::move_if_noexcept(e);
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
        auto index = find_index(new_entry.kv.view.first);

        if (_table[index].used) {
            return { iterator_from_index(index), false };
        } else {
            _table[index] = std::move(new_entry);
            ++_element_count;
            return { iterator_from_index(index), true };
        }
    }

    // insert by key + mapped value
    template <typename KeyParam, typename Mapped>
    std::pair<iterator, bool> insert_element(KeyParam&& key, Mapped&& mapped) {
        check_expand();
        auto index = find_index(key);

        if (_table[index].used) {
            return { iterator_from_index(index), false };
        } else {
            _table[index] = make_entry(std::forward<KeyParam>(key),
                                       std::forward<Mapped>(mapped));
            ++_element_count;
            return { iterator_from_index(index), true };
        }
    }

    // insert or overwrite by key + mapped value
    template <typename KeyParam, typename Mapped>
    std::pair<iterator, bool> insert_assign_element(KeyParam&& key,
                                                    Mapped&& mapped) {
        check_expand();
        auto index = find_index(key);

        if (_table[index].used) {
            _table[index].kv.view.second = std::forward<Mapped>(mapped);
            return { iterator_from_index(index), false };
        } else {
            _table[index] = make_entry(std::forward<KeyParam>(key),
                                       std::forward<Mapped>(mapped));
            ++_element_count;
            return { iterator_from_index(index), true };
        }
    }

    // marks element at given index as in the map
    Value& activate_element(size_type index, const Key& key) {
        _table[index].used = true;
        _table[index].kv.view.first = key;
        ++_element_count;

        return _table[index].kv.view.second;
    }

    std::pair<size_type, iterator> erase_impl(const Key& key) {
        auto index = find_index(key);

        if (!_table[index].used) {
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

    size_type find_index_impl(const Key& key, const table_type& table) const {
        auto index = hash_index_impl(key, table);

        while (table[index].used) {
            if (_key_equal(table[index].kv.view.first, key)) {
                return index;
            }

            index = next_index_impl(index, table);
        }

        return index;
    }

    size_type hash_index(const Key& key) const {
        return hash_index_impl(key, _table);
    }

    size_type hash_index_impl(const Key& key, const table_type& table) const {
        return _hasher(key) & (table.size() - 1);
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
        return { detail::key_value<Key, Value>(std::forward<Args>(args)...),
                 true };
    }

    entry_type empty_slot_factory() const {
        return { detail::key_value<Key, Value>(Key(), Value()), false };
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

#ifdef __cpp_deduction_guides
#include "detail/deduction_guides.hpp"
#endif

template <typename Key, typename Value, typename Hasher, typename KeyEqual,
          typename Allocator>
void swap(dict<Key, Value, Hasher, KeyEqual, Allocator>& A,
          dict<Key, Value, Hasher, KeyEqual, Allocator>& B) {
    A.swap(B);
}

template <typename Key, typename Value, typename Hasher, typename KeyEqual,
          typename Allocator>
bool operator==(const dict<Key, Value, Hasher, KeyEqual, Allocator>& A,
                const dict<Key, Value, Hasher, KeyEqual, Allocator>& B) {
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
bool operator!=(const dict<Key, Value, Hasher, KeyEqual, Allocator>& A,
                const dict<Key, Value, Hasher, KeyEqual, Allocator>& B) {
    return !(A == B);
}

#if __cpp_lib_memory_resource

namespace pmr {
template <typename Key, typename Value, typename Hasher = std::hash<Key>,
          typename KeyEqual = std::equal_to<Key>>
using dict = dict<
    Key, Value, Hasher, KeyEqual,
    std::experimental::pmr::polymorphic_allocator<std::pair<const Key, Value>>>;
} // namespace pmr

#endif

template <typename Hasher>
class murmur_hash_mixer {
public:
    using argument_type = typename Hasher::argument_type;
    using result_type = typename Hasher::result_type;

    murmur_hash_mixer() : hasher_() {}
    murmur_hash_mixer(Hasher hasher) : hasher_(std::move(hasher)) {}

    result_type operator()(const argument_type& key) const {
        result_type ret = hasher_(key);

        ret ^= (ret >> 33);
        ret *= 0xff51afd7ed558ccd;
        ret ^= (ret >> 33);
        ret *= 0xc4ceb9fe1a85ec53;
        ret ^= (ret >> 33);

        return ret;
    }

private:
    Hasher hasher_;
};

} // namespace io

#endif
