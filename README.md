dict
==========
Open-addressed hash map implementation in C++

This is a cache friendly alternative to `std::unordered_map`. Inspired by Chandler Carruth's [talk](https://www.youtube.com/watch?v=fHNmRkzxHWs) at CppCon.

```cpp
#include <iostream>
#include <dict/dict.hpp>

int main() {
  io::dict<std::string, int> worldcups{{"Germany", 4}, {"Brazil", 5}, {"France", 1}};

  std::cout << worldcups["Germany"] << " stars for Germany!" << std::endl;
}
```

The interface is the same as for `std::unordered_map` besides the following list.


### Interface Differences to `std::unordered_map`

 - resides in namespace `io` and is simply called `dict`
 - C++17 [N4279](https://isocpp.org/files/papers/n4279.html) additional member functions
 - no erase(iterator first, iterator last)
 - erase doesn't offer the strong exception safety guarantee and invalidates all iterators
 - no bucket interface (Seriously, who uses that anyway?)
 - the load factor is a percentage - a float in the range of [0,1)

Everything else should be the same as in the standard [unordered_map](http://en.cppreference.com/w/cpp/container/unordered_map).

Follow these links for [implementation](https://github.com/StephanDollberg/dict/tree/master/include/dict) and [performance](https://github.com/StephanDollberg/dict/tree/master/perf) notes.
### Using/Installing dict
`dict` is header only so you don't need to build anything. You can either add `include/dict` to your compiler include path(e.g.: `-I/path/to/dict/include` or put it to `/usr/local/include`) or copy the files to your local project.

#### Dependencies
The only dependency is a recent C++11 compiler and boost. So far we only use the iterator library from boost so the header only part is even enough. Simply install it via your favourite package manager

#### Supported Systems [![Build Status](https://travis-ci.org/StephanDollberg/dict.svg?branch=master)](https://travis-ci.org/StephanDollberg/dict)
Tested on:

- Ubuntu 14.04LTS with default gcc(4.8) and default boost(1.54)
- Fedora 21 with default gcc(4.9) and default boost(1.55)
- Mac OSX with apple clang(3.5svn), clang3.5 and homebrew gcc(4.9) and homebrew boost(1.55)
- Windows 8.1 with STL's mingw(4.9.1) + boost(1.56) pack(12.1)

Contributions and feedback highly appreciated.

### Updates

There is also a branch ([robinhood-ng](https://github.com/StephanDollberg/dict/commits/robinhood-ng)) which implements the robinhood hashing scheme similar to the one in Rust's ` std::collections::HashMap`.
