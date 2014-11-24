dict
==========
Open-addressed hash map implementation in C++


Interface Differences to `std::unordered_map`
===
 - C++17 [N4279](https://isocpp.org/files/papers/n4279.html) additional member functions
 - no erase(iterator first, iterator last)
 - erase doesn't offer strong exception safety
 - no bucket interface

