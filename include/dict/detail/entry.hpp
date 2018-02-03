#ifndef DICT_ENTRY_HPP
#define DICT_ENTRY_HPP

#include "key_value.hpp"

namespace io {

namespace detail {

template <typename Key, typename Value>
struct dict_entry {
    detail::key_value<Key, Value> kv;

    // dict_entry() : kv() {}
    // template<typename KV,
    // 	typename std::enable_if<!std::is_same<KV, dict_entry<Key, Value>>::value, int>::type>
    // dict_entry(KV&& kv) : kv(std::forward<KV>(kv)) {}
};

} // namespace detail

} // namespace io

#endif
