#include "../include/dict/dict.hpp"

#include <unordered_map>
#include <random>
#include <functional>

#ifdef WITH_GOOGLE_BENCH
#include <sparsehash/dense_hash_map>
#endif

#include <benchmark/benchmark.h>


const int lookup_test_size = 1000000;
const int small_lookup_test_size = 10;

struct inc_gen {
    int operator()() { return _counter++; }

    int _counter = 0;
};

// Note that we keep benching on the same instance of the d
// Alternative with creating a per run instance would make us always test on
// cold data
template <typename Map, typename Generator>
void insert_test(Map& map, Generator& gen) {
    for (int i = 0; i != 10000; ++i) {
        map[gen()] = i;
    }
}

static void dict_insert(benchmark::State& state) {
    io::dict<int, int> d;
    d.reserve(20000);

    std::uniform_int_distribution<std::size_t> normal(0, 1000000);
    std::mt19937 engine;
    auto gen = std::bind(std::ref(normal), std::ref(engine));

    while (state.KeepRunning()) {
        insert_test(d, gen);
    }
}
BENCHMARK(dict_insert);

static void umap_insert(benchmark::State& state) {
    std::unordered_map<int, int> d;
    d.reserve(20000);

    std::uniform_int_distribution<std::size_t> normal(0, 1000000);
    std::mt19937 engine;
    auto gen = std::bind(std::ref(normal), std::ref(engine));

    while (state.KeepRunning()) {
        insert_test(d, gen);
    }
}
BENCHMARK(umap_insert);

#ifdef WITH_GOOGLE_BENCH
static void google_insert(benchmark::State& state) {
    google::dense_hash_map<int, int> d(20000);
    d.set_empty_key(0);

    std::uniform_int_distribution<std::size_t> normal(0, 1000000);
    std::mt19937 engine;
    auto gen = std::bind(std::ref(normal), std::ref(engine));

    while (state.KeepRunning()) {
        insert_test(d, gen);
    }
}
BENCHMARK(google_insert);
#endif

#ifdef WITH_GOOGLE_BENCH
template <typename Map, typename Gen>
Map build_map_google(int size, Gen gen) {
    Map d;
    d.set_empty_key(0);

    for (int i = 0; i != size; ++i) {
        d[gen()] = i;
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

template <typename Map, typename Gen>
Map build_map(int size, Gen gen) {
    Map d;

    for (int i = 0; i != size; ++i) {
        d[gen()] = i;
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
int hybrid_test(Map& map, Generator& gen) {
    int res = 0;
    for (std::size_t i = 0; i < 100; i += 1) {
        res += map[gen()];
    }

    return res;
}

// Note that we keep benching on the same instance of the d
// Alternative with creating a per run instance would make us always test on
// cold data
static void dict_hybrid(benchmark::State& state) {
    std::uniform_int_distribution<std::size_t> normal(0,
                                                      2 * lookup_test_size - 1);
    std::mt19937 engine;
    auto gen = std::bind(std::ref(normal), std::ref(engine));
    auto d = build_map<io::dict<int, int>>(lookup_test_size, gen);

    while (state.KeepRunning()) {
        benchmark::DoNotOptimize(hybrid_test(d, gen));
    }
}
BENCHMARK(dict_hybrid);

static void umap_hybrid(benchmark::State& state) {
    std::uniform_int_distribution<std::size_t> normal(0,
                                                      2 * lookup_test_size - 1);
    std::mt19937 engine;
    auto gen = std::bind(std::ref(normal), std::ref(engine));
    auto d = build_map<std::unordered_map<int, int>>(lookup_test_size, gen);

    while (state.KeepRunning()) {
        benchmark::DoNotOptimize(hybrid_test(d, gen));
    }
}
BENCHMARK(umap_hybrid);

#ifdef WITH_GOOGLE_BENCH
static void google_hybrid(benchmark::State& state) {
    std::uniform_int_distribution<std::size_t> normal(0,
                                                      2 * lookup_test_size - 1);
    std::mt19937 engine;
    auto gen = std::bind(std::ref(normal), std::ref(engine));
    auto d = build_map_google<google::dense_hash_map<int, int>>(
        lookup_test_size, gen);

    while (state.KeepRunning()) {
        benchmark::DoNotOptimize(hybrid_test(d, gen));
    }
}
BENCHMARK(google_hybrid);
#endif

// TODO(improve) - this hits us hard
static void dict_hybrid_with_heavy_clustering(benchmark::State& state) {
    auto d = build_map<io::dict<int, int>>(lookup_test_size, inc_gen());

    std::uniform_int_distribution<std::size_t> normal(0, 16 * lookup_test_size -
                                                             1);
    std::mt19937 engine;
    auto gen = std::bind(std::ref(normal), std::ref(engine));

    while (state.KeepRunning()) {
        benchmark::DoNotOptimize(hybrid_test(d, gen));
    }
}
BENCHMARK(dict_hybrid_with_heavy_clustering);

static void umap_hybrid_with_heavy_clustering(benchmark::State& state) {
    auto d =
        build_map<std::unordered_map<int, int>>(lookup_test_size, inc_gen());

    std::uniform_int_distribution<std::size_t> normal(0, 16 * lookup_test_size -
                                                             1);
    std::mt19937 engine;
    auto gen = std::bind(std::ref(normal), std::ref(engine));

    while (state.KeepRunning()) {
        benchmark::DoNotOptimize(hybrid_test(d, gen));
    }
}
BENCHMARK(umap_hybrid_with_heavy_clustering);

#ifdef WITH_GOOGLE_BENCH
static void google_hybrid_with_heavy_clustering(benchmark::State& state) {
    auto d = build_map_google<google::dense_hash_map<int, int>>(
        lookup_test_size, inc_gen());

    std::uniform_int_distribution<std::size_t> normal(0, 16 * lookup_test_size -
                                                             1);
    std::mt19937 engine;
    auto gen = std::bind(std::ref(normal), std::ref(engine));

    while (state.KeepRunning()) {
        benchmark::DoNotOptimize(hybrid_test(d, gen));
    }
}
BENCHMARK(google_hybrid_with_heavy_clustering);
#endif

template <typename Map, typename Generator>
int lookup_test(Map& map, Generator& gen) {
    int res = 0;
    for (std::size_t i = 0; i < 100; i += 1) {
        auto it = map.find(gen());
        if (it != map.end()) {
            res += it->second;
        }
    }

    return res;
}

static void dict_lookup_small(benchmark::State& state) {
    auto d = build_map<io::dict<int, int>>(small_lookup_test_size, inc_gen());

    std::uniform_int_distribution<std::size_t> normal(0, d.size() - 1);
    std::mt19937 engine;
    auto gen = std::bind(std::ref(normal), std::ref(engine));

    while (state.KeepRunning()) {
        benchmark::DoNotOptimize(lookup_test(d, gen));
    }
}
BENCHMARK(dict_lookup_small);

static void umap_lookup_small(benchmark::State& state) {
    auto d = build_map<std::unordered_map<int, int>>(small_lookup_test_size,
                                                     inc_gen());

    std::uniform_int_distribution<std::size_t> normal(0, d.size() - 1);
    std::mt19937 engine;
    auto gen = std::bind(std::ref(normal), std::ref(engine));

    while (state.KeepRunning()) {
        benchmark::DoNotOptimize(lookup_test(d, gen));
    }
}
BENCHMARK(umap_lookup_small);

#ifdef WITH_GOOGLE_BENCH
static void google_lookup_small(benchmark::State& state) {
    auto d = build_map_google<google::dense_hash_map<int, int>>(
        small_lookup_test_size, inc_gen());

    std::uniform_int_distribution<std::size_t> normal(0, d.size() - 1);
    std::mt19937 engine;
    auto gen = std::bind(std::ref(normal), std::ref(engine));

    while (state.KeepRunning()) {
        benchmark::DoNotOptimize(lookup_test(d, gen));
    }
}
BENCHMARK(google_lookup_small);
#endif


static void dict_lookup(benchmark::State& state) {
    std::uniform_int_distribution<std::size_t> normal(0, lookup_test_size - 1);
    std::mt19937 engine;
    auto gen = std::bind(std::ref(normal), std::ref(engine));
    auto d = build_map<io::dict<int, int>>(lookup_test_size, gen);

    while (state.KeepRunning()) {
        benchmark::DoNotOptimize(lookup_test(d, gen));
    }
}
BENCHMARK(dict_lookup);

static void umap_lookup(benchmark::State& state) {
    std::uniform_int_distribution<std::size_t> normal(0, lookup_test_size - 1);
    std::mt19937 engine;
    auto gen = std::bind(std::ref(normal), std::ref(engine));
    auto d = build_map<std::unordered_map<int, int>>(lookup_test_size, gen);

    while (state.KeepRunning()) {
        benchmark::DoNotOptimize(lookup_test(d, gen));
    }
}
BENCHMARK(umap_lookup);

#ifdef WITH_GOOGLE_BENCH
static void google_lookup(benchmark::State& state) {
    std::uniform_int_distribution<std::size_t> normal(0, lookup_test_size - 1);
    std::mt19937 engine;
    auto gen = std::bind(std::ref(normal), std::ref(engine));
    auto d = build_map_google<google::dense_hash_map<int, int>>(
        lookup_test_size, gen);

    while (state.KeepRunning()) {
        benchmark::DoNotOptimize(lookup_test(d, gen));
    }
}
BENCHMARK(google_lookup);
#endif

static void dict_lookup_with_many_misses(benchmark::State& state) {
    std::uniform_int_distribution<std::size_t> build_normal(
        0, lookup_test_size - 1);
    std::mt19937 build_engine;
    auto build_gen = std::bind(std::ref(build_normal), std::ref(build_engine));
    auto d = build_map<io::dict<int, int>>(lookup_test_size, build_gen);

    std::uniform_int_distribution<std::size_t> normal(0, 16 * lookup_test_size -
                                                             1);
    std::mt19937 engine;
    auto gen = std::bind(std::ref(normal), std::ref(engine));

    while (state.KeepRunning()) {
        benchmark::DoNotOptimize(lookup_test(d, gen));
    }
}
BENCHMARK(dict_lookup_with_many_misses);

static void umap_lookup_with_many_misses(benchmark::State& state) {
    std::uniform_int_distribution<std::size_t> build_normal(
        0, lookup_test_size - 1);
    std::mt19937 build_engine;
    auto build_gen = std::bind(std::ref(build_normal), std::ref(build_engine));
    auto d =
        build_map<std::unordered_map<int, int>>(lookup_test_size, build_gen);

    std::uniform_int_distribution<std::size_t> normal(0, 16 * lookup_test_size -
                                                             1);
    std::mt19937 engine;
    auto gen = std::bind(std::ref(normal), std::ref(engine));

    while (state.KeepRunning()) {
        benchmark::DoNotOptimize(lookup_test(d, gen));
    }
}
BENCHMARK(umap_lookup_with_many_misses);

#ifdef WITH_GOOGLE_BENCH
static void google_lookup_with_many_misses(benchmark::State& state) {
    std::uniform_int_distribution<std::size_t> build_normal(
        0, lookup_test_size - 1);
    std::mt19937 build_engine;
    auto build_gen = std::bind(std::ref(build_normal), std::ref(build_engine));
    auto d = build_map_google<google::dense_hash_map<int, int>>(
        lookup_test_size, build_gen);

    std::uniform_int_distribution<std::size_t> normal(0, 16 * d.size() - 1);
    std::mt19937 engine;
    auto gen = std::bind(std::ref(normal), std::ref(engine));

    while (state.KeepRunning()) {
        benchmark::DoNotOptimize(lookup_test(d, gen));
    }
}
BENCHMARK(google_lookup_with_many_misses);
#endif

// TODO(improve) - this hits us hard
static void dict_lookup_with_heavy_clustering(benchmark::State& state) {
    auto d = build_map<io::dict<int, int>>(lookup_test_size, inc_gen());

    std::uniform_int_distribution<std::size_t> normal(0, 16 * lookup_test_size -
                                                             1);
    std::mt19937 engine;
    auto gen = std::bind(std::ref(normal), std::ref(engine));

    while (state.KeepRunning()) {
        benchmark::DoNotOptimize(lookup_test(d, gen));
    }
}
BENCHMARK(dict_lookup_with_heavy_clustering);

static void umap_lookup_with_heavy_clustering(benchmark::State& state) {
    auto d =
        build_map<std::unordered_map<int, int>>(lookup_test_size, inc_gen());

    std::uniform_int_distribution<std::size_t> normal(0, 16 * lookup_test_size -
                                                             1);
    std::mt19937 engine;
    auto gen = std::bind(std::ref(normal), std::ref(engine));

    while (state.KeepRunning()) {
        benchmark::DoNotOptimize(lookup_test(d, gen));
    }
}
BENCHMARK(umap_lookup_with_heavy_clustering);

#ifdef WITH_GOOGLE_BENCH
static void google_lookup_with_heavy_clustering(benchmark::State& state) {
    auto d = build_map_google<google::dense_hash_map<int, int>>(
        lookup_test_size, inc_gen());

    std::uniform_int_distribution<std::size_t> normal(0, 16 * lookup_test_size -
                                                             1);
    std::mt19937 engine;
    auto gen = std::bind(std::ref(normal), std::ref(engine));

    while (state.KeepRunning()) {
        benchmark::DoNotOptimize(lookup_test(d, gen));
    }
}
BENCHMARK(google_lookup_with_heavy_clustering);
#endif

const int build_test_size = 1000;

static void dict_build(benchmark::State& state) {
    while (state.KeepRunning()) {
        benchmark::DoNotOptimize(
            build_map<io::dict<int, int>>(build_test_size, inc_gen()));
    }
}
BENCHMARK(dict_build);

static void umap_build(benchmark::State& state) {
    while (state.KeepRunning()) {
        benchmark::DoNotOptimize(build_map<std::unordered_map<int, int>>(
            build_test_size, inc_gen()));
    }
}
BENCHMARK(umap_build);

#ifdef WITH_GOOGLE_BENCH
static void google_build(benchmark::State& state) {
    while (state.KeepRunning()) {
        // benchmark::DoNotOptimize(
        build_map_google<google::dense_hash_map<int, int>>(build_test_size,
                                                           inc_gen());
        // );
    }
}
BENCHMARK(google_build);
#endif

static void dict_build_with_reserve(benchmark::State& state) {
    while (state.KeepRunning()) {
        benchmark::DoNotOptimize(
            build_map_with_reserve<io::dict<int, int>>(build_test_size));
    }
}
BENCHMARK(dict_build_with_reserve);

static void umap_build_with_reserve(benchmark::State& state) {
    while (state.KeepRunning()) {
        benchmark::DoNotOptimize(
            build_map_with_reserve<std::unordered_map<int, int>>(
                build_test_size));
    }
}
BENCHMARK(umap_build_with_reserve);

#ifdef WITH_GOOGLE_BENCH
static void google_build_with_reserve(benchmark::State& state) {
    while (state.KeepRunning()) {
        // benchmark::DoNotOptimize(
        build_map_google<google::dense_hash_map<int, int>>(build_test_size,
                                                           inc_gen());
        // );
    }
}
BENCHMARK(google_build_with_reserve);
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

static void dict_build_string_keys(benchmark::State& state) {
    while (state.KeepRunning()) {
        benchmark::DoNotOptimize(
            build_string_map<io::dict<std::string, int>>(build_test_size));
    }
}
BENCHMARK(dict_build_string_keys);

static void umap_build_string_keys(benchmark::State& state) {
    while (state.KeepRunning()) {
        benchmark::DoNotOptimize(
            build_string_map<std::unordered_map<std::string, int>>(
                build_test_size));
    }
}
BENCHMARK(umap_build_string_keys);

#ifdef WITH_GOOGLE_BENCH
static void google_build_string_keys(benchmark::State& state) {
    while (state.KeepRunning()) {
        // benchmark::DoNotOptimize(
        build_string_map_google<google::dense_hash_map<std::string, int>>(
            build_test_size);
        // );
    }
}
BENCHMARK(google_build_string_keys);
#endif

template <typename Map, typename LookUpKeys>
int string_lookup_test(Map& map, const LookUpKeys& keys) {
    int res = 0;
    for (auto&& k : keys) {
        res += map.find(k)->second;
    }

    return res;
}

const int string_lookup_test_size = 1000;

static void dict_string_lookup(benchmark::State& state) {
    auto d =
        build_string_map<io::dict<std::string, int>>(string_lookup_test_size);
    std::vector<std::string> keys{ "1111111", "2222222", "3333333",
                                   "4444444", "5555555", "6666666",
                                   "7777777", "8888888", "9999999" };

    while (state.KeepRunning()) {
        benchmark::DoNotOptimize(string_lookup_test(d, keys));
    }
}
BENCHMARK(dict_string_lookup);

static void umap_string_lookup(benchmark::State& state) {
    auto d = build_string_map<std::unordered_map<std::string, int>>(
        string_lookup_test_size);
    std::vector<std::string> keys{ "1111111", "2222222", "3333333",
                                   "4444444", "5555555", "6666666",
                                   "7777777", "8888888", "9999999" };

    while (state.KeepRunning()) {
        benchmark::DoNotOptimize(string_lookup_test(d, keys));
    }
}
BENCHMARK(umap_string_lookup);

#ifdef WITH_GOOGLE_BENCH
static void google_string_lookup(benchmark::State& state) {
    auto d = build_string_map_google<google::dense_hash_map<std::string, int>>(
        string_lookup_test_size);
    std::vector<std::string> keys{ "1111111", "2222222", "3333333",
                                   "4444444", "5555555", "6666666",
                                   "7777777", "8888888", "9999999" };

    while (state.KeepRunning()) {
        benchmark::DoNotOptimize(string_lookup_test(d, keys));
    }
}
BENCHMARK(google_string_lookup);
#endif

int main(int argc, char** argv) {
    benchmark::Initialize(&argc, argv);
    benchmark::RunSpecifiedBenchmarks();
}
