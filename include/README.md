Implementation Notes
===
Currently, the implementation uses linear probing. This allows for immediate deletion of elements.

The number of buckets is a power of two.

In case measurements show that both these properties lead to too strong clustering it should be easy to switch to prime bucket sizes. The code for prime number generation is already in place(from libc++).

Switching to quadratic probing would require a bit more refactoring on the erasure part.

The internal data structure is currently a `std::vector<tuple<bool, pair<Key, Value>>>` where the bool flag indicates whether the element is active. A possible space optimization would be to use an extra bool vector however this would need some more detail analysis(cache etc.).
