#ifndef DICT_ENTRY_HPP
#define DICT_ENTRY_HPP

#include "key_value.hpp"

namespace io {

namespace detail {

template <typename Key, typename Value>
struct dict_entry {
    detail::key_value<Key, Value> kv;
    std::size_t hash;

    dict_entry() : kv(), hash(0) {}
    template<typename KV>
    dict_entry(KV&& kv, std::size_t hash)
        : kv(std::forward<KV>(kv)), hash(hash) {}

    bool entry_is_free() const {
        return hash == 0;
    }
};

} // namespace detail

} // namespace io

#endif
