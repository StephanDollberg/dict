#include <cmath>

namespace io {

namespace detail {

std::size_t next_power_of_two(std::size_t value) {
    return std::pow(2, std::ceil(std::log2(value)));
}

} // detail

} // io
