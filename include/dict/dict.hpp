#ifndef DICT_HPP
#define DICT_HPP

#include <string>
#include <vector>
#include <stdexcept>
#include <tuple>
#include <cmath>
#include <functional>

namespace boost {

template <typename Key, typename Value, typename Hasher = std::hash<Key>,
          typename KeyEqual = std::equal_to<Key>,
          typename Allocator = std::allocator< std::tuple<bool, Key, Value>>>
class dict {
public:
    typedef Key key_type;
    typedef Value mapped_type;
    typedef Allocator allocator_type;
    typedef Hasher hasher;
    typedef KeyEqual key_equal;

    typedef std::pair<Key, Value> value_type;
    typedef std::tuple<bool, Key, Value> entry_type;
    typedef std::vector<entry_type, allocator_type> table_type;
    typedef typename table_type::size_type size_type;
    typedef typename table_type::difference_type difference_type;
    typedef value_type& reference;

    dict()
        : _table(8, std::make_tuple(false, Key(), Value())),
          _element_count(0), _max_element_count(load_factor() * _table.size()) {
    }

    size_type size() const { return _element_count; }

    bool empty() const { return _element_count == 0; }

    // size_type max_size() const {}

    void clear() {
        _table.clear();
        _element_count = 0;
    }

    Value& operator[](const Key& key) {
        auto index = find_index(key);

        if (std::get<0>(_table[index])) {
            return std::get<2>(_table[index]);
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

                auto new_key = hash_index(std::get<1>(_table[delete_index]));

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

            table_type new_table(std::ceil(new_size / load_factor()),
                                 std::make_tuple(false, Key(), Value()));
            new_table.swap(_table);
            _max_element_count = load_factor() * _table.size();
            _element_count = 0;

            for (auto&& e : new_table) {
                if (std::get<0>(e)) {
                    (*this)[std::get<1>(e)] = std::move(std::get<2>(e));
                }
            }
        }
    }

private:

    Value& insert_element(size_type index, const Key& key, Value value) {
        rehash();

        _table[index] = std::make_tuple(true, key, std::move(value));
        ++_element_count;

        return std::get<2>(_table[index]);
    }

    size_type find_index(const Key& key) {
        auto index = hash_index(key);

        while (std::get<0>(_table[index])) {
            if (key_equal{}(std::get<1>(_table[index]), key)) {
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
        return hasher{}(key) % _table.size();
    }

    entry_type empty_slot_factory() const {
        return std::make_tuple(false, Key(), Value());
    }

    table_type _table;
    size_type _element_count;
    size_type _max_element_count;
};

} // namespace boost

#endif
