#define NONIUS_RUNNER
#include <nonius/main.h++>

#include "../include/dict/dict.hpp"

#include <unordered_map>
#include <random>
#include <functional>

#ifdef WITH_GOOGLE_BENCH
#include <sparsehash/dense_hash_map>
#endif

template <typename Map>
void insert_test(Map& map) {
    for (int i = 0; i != 10000; ++i) {
        map[i] = i;
    }
}

NONIUS_BENCHMARK("dict insert", [](nonius::chronometer meter) {
    boost::dict<int, int> d;
    d.reserve(1000);
    meter.measure([&] { insert_test(d); });
})

NONIUS_BENCHMARK("umap insert", [](nonius::chronometer meter) {
    std::unordered_map<int, int> d;
    d.reserve(1000);
    meter.measure([&] { insert_test(d); });
})

#ifdef WITH_GOOGLE_BENCH
NONIUS_BENCHMARK("google insert", [](nonius::chronometer meter) {
    google::dense_hash_map<int, int> d(1000);
    d.set_empty_key(0);
    meter.measure([&] { insert_test(d); });
})
#endif

#ifdef WITH_GOOGLE_BENCH
template <typename Map>
Map build_map_google(int size) {
    Map d;
    d.set_empty_key(0);

    for (int i = 0; i != size; ++i) {
        d[i] = i;
    }

    return d;
}

template <typename Map>
Map build_map_google_with_reserve(int size) {
    Map d(size);
    d.set_empty_key(0);

    for (int i = 0; i != size; ++i) {
        d[i] = i;
    }

    return d;
}
#endif

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
    Map d(size);

    for (int i = 0; i != size; ++i) {
        d[i] = i;
    }

    return d;
}

template <typename Map, typename Generator>
int lookup_test(Map& map, Generator& gen) {
    int res = 0;
    for (std::size_t i = 0; i < 100; i += 1) {
        res += map[gen()];
    }

    return res;
}

const int small_lookup_test_size = 10;

NONIUS_BENCHMARK("small dict lookup", [](nonius::chronometer meter) {
    auto d = build_map<boost::dict<int, int>>(small_lookup_test_size);

    std::uniform_int_distribution<std::size_t> normal(0, d.size() - 1);
    std::mt19937 engine;
    auto gen = std::bind(std::ref(normal), std::ref(engine));

    meter.measure([&, gen] { return lookup_test(d, gen); });
})

NONIUS_BENCHMARK("small umap lookup", [](nonius::chronometer meter) {
    auto d = build_map<std::unordered_map<int, int>>(small_lookup_test_size);

    std::uniform_int_distribution<std::size_t> normal(0, d.size() - 1);
    std::mt19937 engine;
    auto gen = std::bind(std::ref(normal), std::ref(engine));

    meter.measure([&, gen] { return lookup_test(d, gen); });
})

#ifdef WITH_GOOGLE_BENCH
NONIUS_BENCHMARK("small google lookup", [](nonius::chronometer meter) {
    auto d =
        build_map_google<google::dense_hash_map<int, int>>(small_lookup_test_size);

    std::uniform_int_distribution<std::size_t> normal(0, d.size() - 1);
    std::mt19937 engine;
    auto gen = std::bind(std::ref(normal), std::ref(engine));

    meter.measure([&, gen] { return lookup_test(d, gen); });
})
#endif

const int lookup_test_size = 1000000;

NONIUS_BENCHMARK("dict lookup", [](nonius::chronometer meter) {
    auto d = build_map<boost::dict<int, int>>(lookup_test_size);

    std::uniform_int_distribution<std::size_t> normal(0, d.size() - 1);
    std::mt19937 engine;
    auto gen = std::bind(std::ref(normal), std::ref(engine));

    meter.measure([&, gen] { return lookup_test(d, gen); });
})

NONIUS_BENCHMARK("umap lookup", [](nonius::chronometer meter) {
    auto d = build_map<std::unordered_map<int, int>>(lookup_test_size);

    std::uniform_int_distribution<std::size_t> normal(0, d.size() - 1);
    std::mt19937 engine;
    auto gen = std::bind(std::ref(normal), std::ref(engine));

    meter.measure([&, gen] { return lookup_test(d, gen); });
})

#ifdef WITH_GOOGLE_BENCH
NONIUS_BENCHMARK("google lookup", [](nonius::chronometer meter) {
    auto d =
        build_map_google<google::dense_hash_map<int, int>>(lookup_test_size);

    std::uniform_int_distribution<std::size_t> normal(0, d.size() - 1);
    std::mt19937 engine;
    auto gen = std::bind(std::ref(normal), std::ref(engine));

    meter.measure([&, gen] { return lookup_test(d, gen); });
})
#endif

const int build_test_size = 1000;

NONIUS_BENCHMARK("dict build", [] {
    return build_map<boost::dict<int, int>>(build_test_size);
})

NONIUS_BENCHMARK("umap build", [] {
    return build_map<std::unordered_map<int, int>>(build_test_size);
})

#ifdef WITH_GOOGLE_BENCH
NONIUS_BENCHMARK("google build", [] {
    return build_map_google<google::dense_hash_map<int, int>>(build_test_size);
})
#endif

NONIUS_BENCHMARK("dict build with reserve", [] {
    return build_map_with_reserve<boost::dict<int, int>>(build_test_size);
})

NONIUS_BENCHMARK("umap build with reserve", [] {
    return build_map_with_reserve<std::unordered_map<int, int>>(
        build_test_size);
})

#ifdef WITH_GOOGLE_BENCH
NONIUS_BENCHMARK("google build with reserve", [] {
    return build_map_google<google::dense_hash_map<int, int>>(build_test_size);
})
#endif

template <typename Map>
Map build_string_map(int size) {
    Map d;

    for (int i = 0; i != size; ++i) {
        d[std::to_string(i) + std::to_string(i) + std::to_string(i) +
          std::to_string(i) + std::to_string(i) + std::to_string(i) +
          std::to_string(i)] = i;
    }

    return d;
}

#ifdef WITH_GOOGLE_BENCH
template <typename Map>
Map build_string_map_google(int size) {
    Map d;
    d.set_empty_key("");

    for (int i = 0; i != size; ++i) {
        d[std::to_string(i) + std::to_string(i) + std::to_string(i) +
          std::to_string(i) + std::to_string(i) + std::to_string(i) +
          std::to_string(i)] = i;
    }

    return d;
}
#endif

NONIUS_BENCHMARK("dict build with string key", [] {
    return build_string_map<boost::dict<std::string, int>>(build_test_size);
})

NONIUS_BENCHMARK("umap build with string key", [] {
    return build_string_map<std::unordered_map<std::string, int>>(
        build_test_size);
})

#ifdef WITH_GOOGLE_BENCH
NONIUS_BENCHMARK("google build with string key", [] {
    return build_string_map_google<google::dense_hash_map<std::string, int>>(
        build_test_size);
})
#endif

template <typename Map, typename LookUpKeys>
int string_lookup_test(Map& map, const LookUpKeys& keys) {
    int res = 0;
    for (auto&& k : keys) {
        res += map[k];
    }

    return res;
}

const int string_lookup_test_size = 1000;

NONIUS_BENCHMARK("dict string lookup", [](nonius::chronometer meter) {
    auto d = build_string_map<boost::dict<std::string, int>>(
        string_lookup_test_size);
    std::vector<std::string> keys{ "1111111", "2222222", "3333333",
                                   "4444444", "5555555", "6666666",
                                   "7777777", "8888888", "9999999" };

    meter.measure([&] { return string_lookup_test(d, keys); });
})

NONIUS_BENCHMARK("umap string lookup", [](nonius::chronometer meter) {
    auto d = build_string_map<std::unordered_map<std::string, int>>(
        string_lookup_test_size);
    std::vector<std::string> keys{ "1111111", "2222222", "3333333",
                                   "4444444", "5555555", "6666666",
                                   "7777777", "8888888", "9999999" };

    meter.measure([&] { return string_lookup_test(d, keys); });
})

#ifdef WITH_GOOGLE_BENCH
NONIUS_BENCHMARK("google string lookup", [](nonius::chronometer meter) {
    auto d = build_string_map_google<google::dense_hash_map<std::string, int>>(
        string_lookup_test_size);
    std::vector<std::string> keys{ "1111111", "2222222", "3333333",
                                   "4444444", "5555555", "6666666",
                                   "7777777", "8888888", "9999999" };

    meter.measure([&] { return string_lookup_test(d, keys); });
})
#endif
