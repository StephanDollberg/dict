#ifndef DICT_HPP
#define DICT_HPP

#include <vector>
#include <stdexcept>
#include <tuple>
#include <functional>

#include "detail/prime.hpp"
#include "detail/key_value.hpp"
#include "detail/iterator.hpp"

namespace io {

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
    typedef std::tuple<bool, detail::key_value<Key, Value>> entry_type;
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
        return { _table.end(), _table.end(), true };
    }

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

        if (std::get<0>(_table[index])) {
            return iterator_from_index(index);
        } else {
            return end();
        }
    }

    const_iterator find(const Key& key) const {
        auto index = find_index(key);

        if (std::get<0>(_table[index])) {
            return iterator_from_index(index);
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
                if (std::get<0>(e)) {
                    auto new_index =
                        find_index_impl(std::get<1>(e).view.first, new_table);
                    new_table[new_index] = std::move_if_noexcept(e);
                }
            }

            _max_element_count = max_load_factor() * new_table.size();
            _table.swap(new_table);
        }
    }

    void rehash() { reserve(2 * _table.size()); }

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
        auto index = find_index(std::get<1>(new_entry).view.first);

        if (std::get<0>(_table[index])) {
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

        if (std::get<0>(_table[index])) {
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

        if (std::get<0>(_table[index])) {
            std::get<1>(_table[index]).view.second =
                std::forward<Mapped>(mapped);
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
        std::get<0>(_table[index]) = true;
        std::get<1>(_table[index]).view.first = key;
        ++_element_count;

        return std::get<1>(_table[index]).view.second;
    }

    std::pair<size_type, iterator> erase_impl(const Key& key) {
        auto index = find_index(key);

        if (!std::get<0>(_table[index])) {
            return { 0, {} };
        }

        const auto deleted_index = index;

        _table[index] = empty_slot_factory();

        auto delete_index = index;
        while (true) {
            delete_index = next_index(delete_index);

            if (!std::get<0>(_table[delete_index])) {
                return { 1, { std::next(_table.begin(), deleted_index), _table.end() } };
            }

            auto new_key =
                hash_index(std::get<1>(_table[delete_index]).view.first);

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

        while (std::get<0>(table[index])) {
            if (_key_equal(std::get<1>(table[index]).view.first, key)) {
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
        return std::make_tuple(
            true, detail::key_value<Key, Value>(std::forward<Args>(args)...));
    }

    entry_type empty_slot_factory() const {
        return std::make_tuple(false,
                               detail::key_value<Key, Value>(Key(), Value()));
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
