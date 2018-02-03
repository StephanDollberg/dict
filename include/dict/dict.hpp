#ifndef DICT_HPP
#define DICT_HPP

#include <immintrin.h>

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
#include "detail/prime.hpp"


namespace io {

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

// container
template <typename Key, typename Value, typename Hasher = murmur_hash_mixer<std::hash<Key>>,
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
        entry_type>
        _internal_allocator;
    typedef std::vector<entry_type, _internal_allocator> table_type;
    typedef std::vector<std::uint8_t, _internal_allocator> meta_table_type;

    struct find_result {
        std::size_t index;
        std::size_t hash;
    };

    struct hash_split {
        std::size_t higher_bits;
        std::uint8_t lower_bits;
    };

    bool slot_in_use(find_result find) const {
        return _meta_table[find.index] != 0;
    }

    hash_split split_hash(std::size_t hash) const {
        return {hash >> 7, static_cast<uint8_t>(hash | 0x80)};
    }

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
        _meta_table.resize(next_size(initial_size, initial_load_factor()), 0);
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

    iterator begin() noexcept { return { _meta_table.data(), _table.begin(), _table.end() }; }

    const_iterator begin() const noexcept {
        return {_meta_table.data(), _table.begin(), _table.end() };
    }

    const_iterator cbegin() const noexcept {
        return { _meta_table.data(), _table.begin(), _table.end() };
    }

    iterator end() noexcept { return { _meta_table.data(), _table.end(), _table.end(), true }; }

    const_iterator end() const noexcept {
        return { _meta_table.data(), _table.end(), _table.end(), true };
    }

    const_iterator cend() const noexcept {
        return { _meta_table.data(), _table.end(), _table.end(), true };
    }

    size_type size() const noexcept { return _element_count; }

    bool empty() const noexcept { return _element_count == 0; }

    // size_type max_size() const {}

    void clear() {
        // this could optimized to not re-default init everything
        auto old_size = _table.size();
        _table.clear();
        _table.resize(old_size);
        _meta_table.clear();
        _meta_table.resize(old_size, 0);
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
        swap(_meta_table, other._meta_table);
        swap(_element_count, other._element_count);
        swap(_max_element_count, other._max_element_count);
        swap(_key_equal, other._key_equal);
        swap(_hasher, other._hasher);
    }

    iterator find(const Key& key) {
        auto find = find_index(key);

        if (slot_in_use(find)) {
            return iterator_from_index(find.index);
        } else {
            return end();
        }
    }

    const_iterator find(const Key& key) const {
        auto find = find_index(key);

        if (slot_in_use(find)) {
            return iterator_from_index(find.index);
        } else {
            return end();
        }
    }

    Value& at(const Key& key) {
        auto find = find_index(key);

        if (slot_in_use(find)) {
            return _table[find.index].kv.view.second;
        }

        throw std::out_of_range("Key not in dict");
    }

    const Value& at(const Key& key) const {
        auto find = find_index(key);

        if (slot_in_use(find)) {
            return _table[find.index].kv.view.second;
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
        auto find = find_index(key);

        if (slot_in_use(find)) {
            return _table[find.index].kv.view.second;
        } else {
            check_expand();
            return activate_element(key);
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
            meta_table_type new_meta_table(_meta_table.get_allocator());

            new_table.resize(next_size(new_size, max_load_factor()));
            new_meta_table.resize(next_size(new_size, max_load_factor()), 0);

            for (std::size_t index = 0; index < _meta_table.size(); ++index) {
                if (slot_in_use(index)) {
                    auto find =
                        find_index_impl(_table[index].kv.const_view.first,
                            new_table, new_meta_table);
                    new_table[find.index] = std::move_if_noexcept(_table[index]);
                    new_meta_table[find.index] = split_hash(find.hash).lower_bits;
                }
            }

            _max_element_count = max_load_factor() * new_table.size();
            _table.swap(new_table);
            _meta_table.swap(new_meta_table);
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
        auto find = find_index(new_entry.kv.view.first);

        if (slot_in_use(find)) {
            return { iterator_from_index(find.index), false };
        } else {
            _meta_table[find.index] = split_hash(find.hash).lower_bits;
            _table[find.index] = std::move(new_entry);
            ++_element_count;
            return { iterator_from_index(find.index), true };
        }
    }

    // insert by key + mapped value
    template <typename KeyParam, typename Mapped>
    std::pair<iterator, bool> insert_element(KeyParam&& key, Mapped&& mapped) {
        check_expand();
        auto find = find_index(key);

        if (slot_in_use(find)) {
            return { iterator_from_index(find.index), false };
        } else {
            _meta_table[find.index] = split_hash(find.hash).lower_bits;
            _table[find.index] = make_entry(std::forward<KeyParam>(key),
                                       std::forward<Mapped>(mapped));
            ++_element_count;
            return { iterator_from_index(find.index), true };
        }
    }

    // insert or overwrite by key + mapped value
    template <typename KeyParam, typename Mapped>
    std::pair<iterator, bool> insert_assign_element(KeyParam&& key,
                                                    Mapped&& mapped) {
        check_expand();
        auto find = find_index(key);

        if (slot_in_use(find)) {
            _table[find.index].kv.view.second = std::forward<Mapped>(mapped);
            return { iterator_from_index(find.index), false };
        } else {
            _meta_table[find.index] = split_hash(find.hash).lower_bits;
            _table[find.index] = make_entry(std::forward<KeyParam>(key),
                                       std::forward<Mapped>(mapped));
            ++_element_count;
            return { iterator_from_index(find.index), true };
        }
    }

    // marks element at given index as in the map
    Value& activate_element(const Key& key) {
        auto find = find_index(key);
        _meta_table[find.index] = split_hash(find.hash).lower_bits;
        _table[find.index].kv.view.first = key;
        ++_element_count;

        return _table[find.index].kv.view.second;
    }

    std::pair<size_type, iterator> erase_impl(const Key& key) {
        auto find = find_index(key);
        auto index = find.index;

        if (!slot_in_use(find)) {
            return { 0, {} };
        }

        const auto deleted_index = index;

        _table[index] = empty_slot_factory();
        _meta_table[index] = 0;
        --_element_count;

        auto delete_index = index;
        while (true) {
            delete_index = next_index(delete_index);

            if (!slot_in_use(delete_index)) {
                return { 1,
                         {std::next(_meta_table.data(), deleted_index),
                            std::next(_table.begin(), deleted_index), _table.end()} };
            }

            auto new_key = hash_index(
                split_hash(safe_hash(_table[delete_index].kv.view.first)).higher_bits);

            if ((index <= delete_index)
                    ? ((index < new_key) && (new_key <= delete_index))
                    : ((index < new_key) || (new_key <= delete_index))) {
                continue;
            }

            // swapping previously emptied index with delete_index
            using std::swap;
            swap(_table[index], _table[delete_index]);
            swap(_meta_table[index], _meta_table[delete_index]);
            index = delete_index;
        }
    }

    find_result find_index(const Key& key) const {
        return find_index_impl(key, _table, _meta_table);
    }

    find_result find_index_impl(const Key& key,
        const table_type& table, const meta_table_type& meta_table) const {
        auto hash = safe_hash(key);
        return find_index_impl_with_hash(hash, key, table, meta_table);
    }

    find_result find_index_impl_with_hash(std::size_t hash, const Key& key,
        const table_type& table, const meta_table_type& meta_table) const {
        auto hash_split = split_hash(hash);

        auto index = hash_index_impl(hash_split.higher_bits, table);
        auto index_offset = index % 32;
        index -= index_offset;


        while(true) {
            __m256i hash_mask = _mm256_set1_epi8(hash_split.lower_bits);
            __m256i zero_mask = _mm256_set1_epi8(0);

            __m256i meta_vector = _mm256_loadu_si256(
                reinterpret_cast<__m256i const*>(&meta_table[index]));

            __m256i hash_cmp_result = _mm256_cmpeq_epi8(hash_mask, meta_vector);
            int hash_cmp_mask = _mm256_movemask_epi8(hash_cmp_result);
            __m256i zero_cmp_result = _mm256_cmpeq_epi8(zero_mask, meta_vector);
            int zero_cmp_mask = _mm256_movemask_epi8(zero_cmp_result);

            while (hash_cmp_mask != 0) {
                int bitpos = __builtin_ctz(hash_cmp_mask);
                std::size_t local_index = index + bitpos;

                if (_key_equal(table[local_index].kv.view.first, key)) {
                    return {local_index, hash};
                }

                hash_cmp_mask = hash_cmp_mask & ~(1llu << bitpos);
            }

            zero_cmp_mask = zero_cmp_mask & ~((1llu << index_offset) - 1);
            if (zero_cmp_mask != 0) {
                std::size_t offset =  __builtin_ctz(zero_cmp_mask);
                return {index + offset, hash};
            }

            for (std::size_t i = 0; i < 32; ++i) {
                index = next_index_impl(index, table);
            }

            index_offset = 0;
        }
    }

    size_type hash_index(std::size_t hash) const {
        return hash_index_impl(hash, _table);
    }

    size_type hash_index_impl(std::size_t hash, const table_type& table) const {
        return hash & (table.size() - 1);
    }

    // inspired by Rust HashMap
    // we set MSB to one as 0 is special value to indicate empty element
    size_type safe_hash(const Key& key) const {
        return _hasher(key);
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
        return { detail::key_value<Key, Value>(std::forward<Args>(args)...) };
    }

    entry_type empty_slot_factory() const {
        return { detail::key_value<Key, Value>(Key(), Value()) };
    }

    iterator iterator_from_index(size_type index) {
        return { std::next(_meta_table.data(), index),
            std::next(_table.begin(), index), _table.end(), true };
    }

    const_iterator iterator_from_index(size_type index) const {
        return { std::next(_meta_table.data(), index),
            std::next(_table.begin(), index), _table.end(), true };
    }

    size_type initial_size() const { return detail::next_power_of_two(32); }

    constexpr size_type next_size(size_type min_size,
                                  double load_factor) const {
        return detail::next_power_of_two(std::ceil(min_size / load_factor));
    }

    constexpr float initial_load_factor() const { return 0.7; }

    bool slot_in_use(std::size_t index) const {
        return slot_in_use_impl(index, _meta_table);
    }

    bool slot_in_use_impl(std::size_t index, const meta_table_type& meta_table) const {
        return meta_table[index] != 0;
    }

    table_type _table;
    meta_table_type _meta_table;
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

#if __cpp_lib_memory_resource

namespace pmr {
template <typename Key, typename Value, typename Hasher = std::hash<Key>,
          typename KeyEqual = std::equal_to<Key>>
using dict = dict<
    Key, Value, Hasher, KeyEqual,
    std::experimental::pmr::polymorphic_allocator<std::pair<const Key, Value>>>;
} // namespace pmr

#endif

} // namespace io

#endif
