dict
==========
Open-addressed hash map implementation in C++

This is a cache friendly alternative to `std::unordered_map`. 

```
#include <iostream>
#include <dict/dict.hpp>

int main() {
  io::dict<std::string, int> worldcups{{"Germany", 4}, {"Brazil", 5}, {"France", 2}};
	
  std::cout << "Germany won the world cup " << worldcups["Germany"] << " times " << std::endl;
}
```

The interface is the same as for `std::unordered_map` besides the following list.


### Interface Differences to `std::unordered_map`

 - resides in namespace `io`
 - C++17 [N4279](https://isocpp.org/files/papers/n4279.html) additional member functions
 - no erase(iterator first, iterator last)
 - erase doesn't offer the strong exception safety guarantee
 - no bucket interface

Everything else should be the [same](http://en.cppreference.com/w/cpp/container/unordered_map).
 
### Using/Installing dict
`dict` is header only so you don't need to build anything. You can either add `include/dict` to your compiler include path(e.g.: `-I/path/to/dict/include` or put it to `/usr/local/include`) or copy the files to your local project.

#### Dependencies
The only dependency is a recent C++11 compiler and boost. So far we only use the iterator library from boost so the header only part is even enough. Simply install it via your favourite package manager

#### Supported Systems
Tested on:

- Ubuntu 14.04LTS with default gcc(4.8) and default boost(1.54) 
- Fedora 21 with default gcc(4.9) and default boost(1.55)
- Mac OSX with apple clang(3.5svn) and homebrew gcc(4.9) and homebrew boost(1.55) 
