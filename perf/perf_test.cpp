#define NONIUS_RUNNER
#include <nonius/main.h++>

#include "../include/dict/dict.hpp"

#include <unordered_map>
#include <random>
#include <functional>

template <typename Map>
void insert_test(Map& map) {
    for (int i = 0; i != 10000; ++i) {
        map[i] = i;
    }

    // map[42] = 23;
    // map[43] = 23;
    // map[44] = 23;
    // map[45] = 23;
    // map[46] = 23;
    // map[47] = 23;
    // map[48] = 23;
    // map[49] = 23;
}

// #include <bam/timer.hpp>

// NONIUS_BENCHMARK("dict insert", [](nonius::chronometer meter) {
//     boost::dict<int, int> d;
//     d.reserve(1000);
//     meter.measure([&] {
//         insert_test(d);
//     });
// })

// NONIUS_BENCHMARK("umap insert", [](nonius::chronometer meter) {
//     std::unordered_map<int, int> d;
//     d.reserve(1000);
//     meter.measure([&] {
//         insert_test(d);
//     });
// })

// NONIUS_BENCHMARK("dict update", [](nonius::chronometer meter) {
//     boost::dict<int, int> d;
//     d[42] = 1;
//     meter.measure([&] {
//         d[42] = 42;
//     });
// })

// NONIUS_BENCHMARK("umap update", [](nonius::chronometer meter) {
//     std::unordered_map<int, int> d;
//     d[42] = 1;
//     meter.measure([&] {
//         d[42] = 42;
//     });
// })

template <typename Map>
Map build_map(int size) {
    Map d;

    for (int i = 0; i != size; ++i) {
        d[i] = i;
    }

    return d;
}

template <typename Map>
Map build_map_with_reserve(int size) {
    Map d;
    d.reserve(size);

    for (int i = 0; i != size; ++i) {
        d[i] = i;
    }

    return d;
}

template <typename Map, typename Generator>
void lookup_test(Map& map, Generator& gen) {
    for (std::size_t i = 0; i < 100; i += 1) {
        map[gen()];
    }
}

// #include <sparsehash/dense_hash_map>

// google
// string key
// random lookup

const int lookup_test_size = 1000000;

NONIUS_BENCHMARK("dict lookup", [](nonius::chronometer meter) {
    auto d = build_map<boost::dict<int, int>>(lookup_test_size);

    std::uniform_int_distribution<std::size_t> normal(0, d.size() - 1);
    std::mt19937 engine;
    auto gen = std::bind(std::ref(normal), std::ref(engine));

    meter.measure([&, gen] { lookup_test(d, gen); });
})

NONIUS_BENCHMARK("umap lookup", [](nonius::chronometer meter) {
    auto d = build_map<std::unordered_map<int, int>>(lookup_test_size);

    std::uniform_int_distribution<std::size_t> normal(0, d.size() - 1);
    std::mt19937 engine;
    auto gen = std::bind(std::ref(normal), std::ref(engine));

    meter.measure([&, gen] { lookup_test(d, gen); });
})

const int build_test_size = 1000;

NONIUS_BENCHMARK("dict build", [] {
    return build_map<boost::dict<int, int>>(build_test_size);
})

NONIUS_BENCHMARK("umap build", [] {
    return build_map<std::unordered_map<int, int>>(build_test_size);
})

NONIUS_BENCHMARK("dict build with reserve", [] {
    return build_map_with_reserve<boost::dict<int, int>>(build_test_size);
})

NONIUS_BENCHMARK("umap build with reserve", [] {
    return build_map_with_reserve<std::unordered_map<int, int>>(build_test_size);
})
