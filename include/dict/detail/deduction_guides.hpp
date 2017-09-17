namespace detail {

template <class InputIt>
using iter_key_t = std::remove_const_t<
    typename std::iterator_traits<InputIt>::value_type::first_type>;

template <class InputIt>
using iter_val_t =
    typename std::iterator_traits<InputIt>::value_type::second_type;

template <class InputIt>
using iter_to_alloc_t =
    std::pair<std::add_const_t<typename std::iterator_traits<
                  InputIt>::value_type::first_type>,
              typename std::iterator_traits<InputIt>::value_type::second_type>;

} // namespace detail

template <class InputIt, class Hash = std::hash<detail::iter_key_t<InputIt>>,
          class Pred = std::equal_to<detail::iter_key_t<InputIt>>,
          class Alloc = std::allocator<detail::iter_to_alloc_t<InputIt>>>
dict(InputIt, InputIt, std::size_t = std::size_t(), Hash = Hash(),
     Pred = Pred(), Alloc = Alloc())
    ->dict<detail::iter_key_t<InputIt>, detail::iter_val_t<InputIt>, Hash, Pred,
           Alloc>;

template <class Key, class T, class Hash = std::hash<Key>,
          class Pred = std::equal_to<Key>,
          class Alloc = std::allocator<std::pair<const Key, T>>>
dict(std::initializer_list<std::pair<const Key, T>>,
     typename dict<Key, T, Hash, Pred, Alloc>::size_type =
         dict<Key, T, Hash, Pred, Alloc>::size_type(),
     Hash = Hash(), Pred = Pred(), Alloc = Alloc())
    ->dict<Key, T, Hash, Pred, Alloc>;

template <class InputIt, class Alloc>
dict(
    InputIt, InputIt,
    typename dict<detail::iter_key_t<InputIt>, detail::iter_val_t<InputIt>,
                  std::hash<detail::iter_key_t<InputIt>>,
                  std::equal_to<detail::iter_key_t<InputIt>>, Alloc>::size_type,
    Alloc)
    ->dict<detail::iter_key_t<InputIt>, detail::iter_val_t<InputIt>,
           std::hash<detail::iter_key_t<InputIt>>,
           std::equal_to<detail::iter_key_t<InputIt>>, Alloc>;

template <class InputIt, class Alloc>
dict(InputIt, InputIt, Alloc)
    ->dict<detail::iter_key_t<InputIt>, detail::iter_val_t<InputIt>,
           std::hash<detail::iter_key_t<InputIt>>,
           std::equal_to<detail::iter_key_t<InputIt>>, Alloc>;

template <class InputIt, class Hash, class Alloc>
dict(InputIt, InputIt,
     typename dict<detail::iter_key_t<InputIt>, detail::iter_val_t<InputIt>,
                   Hash, std::equal_to<detail::iter_key_t<InputIt>>,
                   Alloc>::size_type,
     Hash, Alloc)
    ->dict<detail::iter_key_t<InputIt>, detail::iter_val_t<InputIt>, Hash,
           std::equal_to<detail::iter_key_t<InputIt>>, Alloc>;

template <class Key, class T, typename Alloc>
dict(
    std::initializer_list<std::pair<const Key, T>>,
    typename dict<Key, T, std::hash<Key>, std::equal_to<Key>, Alloc>::size_type,
    Alloc)
    ->dict<Key, T, std::hash<Key>, std::equal_to<Key>, Alloc>;

template <class Key, class T, typename Alloc>
dict(std::initializer_list<std::pair<const Key, T>>, Alloc)
    ->dict<Key, T, std::hash<Key>, std::equal_to<Key>, Alloc>;

template <class Key, class T, class Hash, class Alloc>
dict(std::initializer_list<std::pair<const Key, T>>,
     typename dict<Key, T, Hash, std::equal_to<Key>, Alloc>::size_type, Hash,
     Alloc)
    ->dict<Key, T, Hash, std::equal_to<Key>, Alloc>;
