#ifndef DICT_ENTRY_HPP
#define DICT_ENTRY_HPP

#include "key_value.hpp"

namespace io {

namespace detail {

template <typename Key, typename Value>
struct dict_entry {
    detail::key_value<Key, Value> kv;
    bool used;

    dict_entry() : kv(), used(false) {}
    template<typename KV>
    dict_entry(KV&& kv, bool used) : kv(std::forward<KV>(kv)), used(used) {}
};

} // namespace detail

} // namespace io

#endif
