#include "../include/dict/dict.hpp"

#include <algorithm>
#include <functional>
#include <random>
#include <unordered_map>

#ifdef WITH_GOOGLE_BENCH
#include <sparsehash/dense_hash_map>
#endif

#include <benchmark/benchmark.h>

void murmur_hash_mixer(benchmark::State& state) {
    auto mixer = io::murmur_hash_mixer<std::hash<uint64_t>>{};
    std::uniform_int_distribution<std::size_t> normal(0, std::numeric_limits<uint64_t>::max());
    std::mt19937 engine;
    auto gen = std::bind(std::ref(normal), std::ref(engine));

    for (auto __attribute__((unused)) _ : state) {
        state.PauseTiming();
        std::array<uint64_t, 100> insert_vals;
        std::generate(insert_vals.begin(), insert_vals.end(), gen);
        state.ResumeTiming();

        for (auto val : insert_vals) {
            benchmark::DoNotOptimize(mixer(val));
        }
    }
}
BENCHMARK(murmur_hash_mixer);

#define BENCH_SIZES ->Arg(8)->Arg(8 << 10)->Arg(8 << 14)->Arg(8 << 20)
#define COLLISION_BENCH_SIZES ->Arg(8)->Arg(8 << 5)->Arg(8 << 10)

struct inc_gen {
    std::size_t operator()() { return _counter++; }

    std::size_t _counter = 0;
};

struct collision_hasher {
    std::size_t operator()(std::size_t) const { return 1; }
};

template <typename Map, typename Container>
void insert_test_impl(Map& map, Container& insert_vals) {
    std::size_t i = 0;
    for (auto&& insert_val: insert_vals) {
        map[insert_val] = i++;
    }
}

template <typename State, typename Map, typename Generator>
void insert_test(State& state, Map& map, Generator& gen) {
    for (auto __attribute__((unused)) _ : state) {
        state.PauseTiming();
        auto temporary_map = map;
        std::array<std::size_t, 100> insert_vals;
        std::generate(insert_vals.begin(), insert_vals.end(), gen);
        state.ResumeTiming();
        insert_test_impl(temporary_map, insert_vals);
    }
}

static void dict_insert(benchmark::State& state) {
    const std::size_t test_size = state.range(0);
    io::dict<std::size_t, std::size_t> d;
    d.reserve(test_size);

    std::uniform_int_distribution<std::size_t> normal(0, 2 * test_size);
    std::mt19937 engine;
    auto gen = std::bind(std::ref(normal), std::ref(engine));

    insert_test(state, d, gen);
}
BENCHMARK(dict_insert)
BENCH_SIZES;

static void umap_insert(benchmark::State& state) {
    const std::size_t test_size = state.range(0);
    std::unordered_map<std::size_t, std::size_t> d;
    d.reserve(test_size);

    std::uniform_int_distribution<std::size_t> normal(0, 2 * test_size);
    std::mt19937 engine;
    auto gen = std::bind(std::ref(normal), std::ref(engine));

    insert_test(state, d, gen);
}
BENCHMARK(umap_insert)
BENCH_SIZES;

static void map_insert(benchmark::State& state) {
    const std::size_t test_size = state.range(0);
    std::map<std::size_t, std::size_t> d;

    std::uniform_int_distribution<std::size_t> normal(0, 2 * test_size);
    std::mt19937 engine;
    auto gen = std::bind(std::ref(normal), std::ref(engine));

    insert_test(state, d, gen);
}
BENCHMARK(map_insert)
BENCH_SIZES;

#ifdef WITH_GOOGLE_BENCH
static void goog_insert(benchmark::State& state) {
    const std::size_t test_size = state.range(0);
    google::dense_hash_map<std::size_t, std::size_t> d(test_size);
    d.set_empty_key(0);

    std::uniform_int_distribution<std::size_t> normal(0, 2 * test_size);
    std::mt19937 engine;
    auto gen = std::bind(std::ref(normal), std::ref(engine));

    insert_test(state, d, gen);
}
BENCHMARK(goog_insert)
BENCH_SIZES;
#endif

#ifdef WITH_GOOGLE_BENCH
template <typename Map, typename Gen>
Map build_map_google(std::size_t size, Gen gen) {
    Map d;
    d.set_empty_key(0);

    for (std::size_t i = 0; i != size; ++i) {
        d[gen()] = i;
    }

    return d;
}

template <typename Map>
Map build_map_google_with_reserve(std::size_t size) {
    Map d(size);
    d.set_empty_key(0);

    for (std::size_t i = 0; i != size; ++i) {
        d[i] = i;
    }

    return d;
}
#endif

template <typename Map, typename Gen>
Map build_map(std::size_t size, Gen gen) {
    Map d;

    for (std::size_t i = 0; i != size; ++i) {
        d[gen()] = i;
    }

    return d;
}

template <typename Map>
Map build_map_with_reserve(std::size_t size) {
    Map d(size);

    for (std::size_t i = 0; i != size; ++i) {
        d[i] = i;
    }

    return d;
}

template <typename Map, typename Container>
std::size_t hybrid_test_impl(Map& map, Container& test_vals) {
    std::size_t res = 0;
    for (auto&& test_val: test_vals) {
        res += map[test_val];
    }

    return res;
}

template <typename State, typename Map, typename Generator>
void hybrid_test(State& state, Map& map, Generator& gen) {
    for (auto __attribute__((unused)) _ : state) {
        state.PauseTiming();
        auto temporary_map = map;
        std::array<std::size_t, 100> test_vals;
        std::generate(test_vals.begin(), test_vals.end(), gen);
        state.ResumeTiming();
        benchmark::DoNotOptimize(hybrid_test_impl(temporary_map, test_vals));
    }
}

static void dict_hybrid(benchmark::State& state) {
    const std::size_t test_size = state.range(0);
    std::uniform_int_distribution<std::size_t> normal(0, 2 * test_size - 1);
    std::mt19937 engine;
    auto gen = std::bind(std::ref(normal), std::ref(engine));
    auto d = build_map<io::dict<std::size_t, std::size_t>>(test_size, gen);

    hybrid_test(state, d, gen);
}
BENCHMARK(dict_hybrid)
BENCH_SIZES;

static void umap_hybrid(benchmark::State& state) {
    const std::size_t test_size = state.range(0);
    std::uniform_int_distribution<std::size_t> normal(0, 2 * test_size - 1);
    std::mt19937 engine;
    auto gen = std::bind(std::ref(normal), std::ref(engine));
    auto d = build_map<std::unordered_map<std::size_t, std::size_t>>(test_size, gen);

    hybrid_test(state, d, gen);
}
BENCHMARK(umap_hybrid)
BENCH_SIZES;

#ifdef WITH_GOOGLE_BENCH
static void goog_hybrid(benchmark::State& state) {
    const std::size_t test_size = state.range(0);
    std::uniform_int_distribution<std::size_t> normal(0, 2 * test_size - 1);
    std::mt19937 engine;
    auto gen = std::bind(std::ref(normal), std::ref(engine));
    auto d = build_map_google<google::dense_hash_map<std::size_t, std::size_t>>(test_size, gen);

    hybrid_test(state, d, gen);
}
BENCHMARK(goog_hybrid)
BENCH_SIZES;
#endif

// TODO(improve) - this hits us hard
static void dict_hybrid_with_heavy_clustering(benchmark::State& state) {
    const std::size_t test_size = state.range(0);
    auto d = build_map<io::dict<std::size_t, std::size_t>>(test_size, inc_gen());

    std::uniform_int_distribution<std::size_t> normal(0, 16 * test_size - 1);
    std::mt19937 engine;
    auto gen = std::bind(std::ref(normal), std::ref(engine));

    hybrid_test(state, d, gen);
}
BENCHMARK(dict_hybrid_with_heavy_clustering)
BENCH_SIZES;

static void umap_hybrid_with_heavy_clustering(benchmark::State& state) {
    const std::size_t test_size = state.range(0);
    auto d = build_map<std::unordered_map<std::size_t, std::size_t>>(test_size, inc_gen());

    std::uniform_int_distribution<std::size_t> normal(0, 16 * test_size - 1);
    std::mt19937 engine;
    auto gen = std::bind(std::ref(normal), std::ref(engine));

    hybrid_test(state, d, gen);
}
BENCHMARK(umap_hybrid_with_heavy_clustering)
BENCH_SIZES;

#ifdef WITH_GOOGLE_BENCH
static void goog_hybrid_with_heavy_clustering(benchmark::State& state) {
    const std::size_t test_size = state.range(0);
    auto d = build_map_google<google::dense_hash_map<std::size_t, std::size_t>>(test_size,
                                                                inc_gen());

    std::uniform_int_distribution<std::size_t> normal(0, 16 * test_size - 1);
    std::mt19937 engine;
    auto gen = std::bind(std::ref(normal), std::ref(engine));

    hybrid_test(state, d, gen);
}
BENCHMARK(goog_hybrid_with_heavy_clustering)
BENCH_SIZES;
#endif

static void dict_hybrid_with_only_collisions(benchmark::State& state) {
    const std::size_t test_size = state.range(0);
    auto d =
        build_map<io::dict<std::size_t, std::size_t, collision_hasher>>(test_size, inc_gen());

    std::uniform_int_distribution<std::size_t> normal(0, 16 * test_size - 1);
    std::mt19937 engine;
    auto gen = std::bind(std::ref(normal), std::ref(engine));

    hybrid_test(state, d, gen);
}
BENCHMARK(dict_hybrid_with_only_collisions)
COLLISION_BENCH_SIZES;

static void umap_hybrid_with_only_collisions(benchmark::State& state) {
    const std::size_t test_size = state.range(0);
    auto d = build_map<std::unordered_map<std::size_t, std::size_t, collision_hasher>>(
        test_size, inc_gen());

    std::uniform_int_distribution<std::size_t> normal(0, 16 * test_size - 1);
    std::mt19937 engine;
    auto gen = std::bind(std::ref(normal), std::ref(engine));

    hybrid_test(state, d, gen);
}
BENCHMARK(umap_hybrid_with_only_collisions)
COLLISION_BENCH_SIZES;

#ifdef WITH_GOOGLE_BENCH
static void goog_hybrid_with_only_collisions(benchmark::State& state) {
    const std::size_t test_size = state.range(0);
    auto d =
        build_map_google<google::dense_hash_map<std::size_t, std::size_t, collision_hasher>>(
            test_size, inc_gen());

    std::uniform_int_distribution<std::size_t> normal(0, 16 * test_size - 1);
    std::mt19937 engine;
    auto gen = std::bind(std::ref(normal), std::ref(engine));

    hybrid_test(state, d, gen);
}
BENCHMARK(goog_hybrid_with_only_collisions)
COLLISION_BENCH_SIZES;
#endif

template <typename Map, typename Container>
std::size_t lookup_test_impl(Map& map, Container& lookup_vals) {
    std::size_t res = 0;
    for (auto&& val: lookup_vals) {
        auto it = map.find(val);
        if (it != map.end()) {
            res += it->second;
        }
    }

    return res;
}

template <typename State, typename Map, typename Generator>
void lookup_test(State& state, Map& map, Generator& gen) {
    std::array<std::size_t, 100> lookup_vals;
    for (auto __attribute__((unused)) _ : state) {
        state.PauseTiming();
        std::generate(lookup_vals.begin(), lookup_vals.end(), gen);
        state.ResumeTiming();
        benchmark::DoNotOptimize(lookup_test_impl(map, lookup_vals));
    }
}

static void dict_lookup(benchmark::State& state) {
    const std::size_t test_size = state.range(0);
    std::uniform_int_distribution<std::size_t> normal(0, 2 * test_size - 1);
    std::mt19937 engine;
    auto gen = std::bind(std::ref(normal), std::ref(engine));
    auto d = build_map<io::dict<std::size_t, std::size_t>>(test_size, gen);

    lookup_test(state, d, gen);
}
BENCHMARK(dict_lookup)
BENCH_SIZES;

static void dict_with_finalizer_lookup(benchmark::State& state) {
    const std::size_t test_size = state.range(0);
    std::uniform_int_distribution<std::size_t> normal(0, 2 * test_size - 1);
    std::mt19937 engine;
    auto gen = std::bind(std::ref(normal), std::ref(engine));
    auto d = build_map<io::dict<std::size_t, std::size_t, io::murmur_hash_mixer<std::hash<std::size_t>>>>(test_size, gen);

    lookup_test(state, d, gen);
}
BENCHMARK(dict_with_finalizer_lookup)
BENCH_SIZES;

static void umap_lookup(benchmark::State& state) {
    const std::size_t test_size = state.range(0);
    std::uniform_int_distribution<std::size_t> normal(0, 2 * test_size - 1);
    std::mt19937 engine;
    auto gen = std::bind(std::ref(normal), std::ref(engine));
    auto d = build_map<std::unordered_map<std::size_t, std::size_t>>(test_size, gen);

    lookup_test(state, d, gen);
}
BENCHMARK(umap_lookup)
BENCH_SIZES;

static void map_lookup(benchmark::State& state) {
    const std::size_t test_size = state.range(0);
    std::uniform_int_distribution<std::size_t> normal(0, 2 * test_size - 1);
    std::mt19937 engine;
    auto gen = std::bind(std::ref(normal), std::ref(engine));
    auto d = build_map<std::map<std::size_t, std::size_t>>(test_size, gen);

    lookup_test(state, d, gen);
}
BENCHMARK(map_lookup)
BENCH_SIZES;

#ifdef WITH_GOOGLE_BENCH
static void goog_lookup(benchmark::State& state) {
    const std::size_t test_size = state.range(0);
    std::uniform_int_distribution<std::size_t> normal(0, 2 * test_size - 1);
    std::mt19937 engine;
    auto gen = std::bind(std::ref(normal), std::ref(engine));
    auto d = build_map_google<google::dense_hash_map<std::size_t, std::size_t>>(test_size, gen);

    lookup_test(state, d, gen);
}
BENCHMARK(goog_lookup)
BENCH_SIZES;
#endif

static void dict_lookup_with_many_misses(benchmark::State& state) {
    const std::size_t test_size = state.range(0);
    std::uniform_int_distribution<std::size_t> build_normal(0, test_size - 1);
    std::mt19937 build_engine;
    auto build_gen = std::bind(std::ref(build_normal), std::ref(build_engine));
    auto d = build_map<io::dict<std::size_t, std::size_t>>(test_size, build_gen);

    std::uniform_int_distribution<std::size_t> normal(0, 16 * test_size - 1);
    std::mt19937 engine;
    auto gen = std::bind(std::ref(normal), std::ref(engine));

    lookup_test(state, d, gen);
}
BENCHMARK(dict_lookup_with_many_misses)
BENCH_SIZES;

static void umap_lookup_with_many_misses(benchmark::State& state) {
    const std::size_t test_size = state.range(0);
    std::uniform_int_distribution<std::size_t> build_normal(0, test_size - 1);
    std::mt19937 build_engine;
    auto build_gen = std::bind(std::ref(build_normal), std::ref(build_engine));
    auto d = build_map<std::unordered_map<std::size_t, std::size_t>>(test_size, build_gen);

    std::uniform_int_distribution<std::size_t> normal(0, 16 * test_size - 1);
    std::mt19937 engine;
    auto gen = std::bind(std::ref(normal), std::ref(engine));

    lookup_test(state, d, gen);
}
BENCHMARK(umap_lookup_with_many_misses)
BENCH_SIZES;

static void map_lookup_with_many_misses(benchmark::State& state) {
    const std::size_t test_size = state.range(0);
    std::uniform_int_distribution<std::size_t> build_normal(0, test_size - 1);
    std::mt19937 build_engine;
    auto build_gen = std::bind(std::ref(build_normal), std::ref(build_engine));
    auto d = build_map<std::map<std::size_t, std::size_t>>(test_size, build_gen);

    std::uniform_int_distribution<std::size_t> normal(0, 16 * test_size - 1);
    std::mt19937 engine;
    auto gen = std::bind(std::ref(normal), std::ref(engine));

    lookup_test(state, d, gen);
}
BENCHMARK(map_lookup_with_many_misses)
BENCH_SIZES;

#ifdef WITH_GOOGLE_BENCH
static void goog_lookup_with_many_misses(benchmark::State& state) {
    const std::size_t test_size = state.range(0);
    std::uniform_int_distribution<std::size_t> build_normal(0, test_size - 1);
    std::mt19937 build_engine;
    auto build_gen = std::bind(std::ref(build_normal), std::ref(build_engine));
    auto d = build_map_google<google::dense_hash_map<std::size_t, std::size_t>>(test_size,
                                                                build_gen);

    std::uniform_int_distribution<std::size_t> normal(0, 16 * d.size() - 1);
    std::mt19937 engine;
    auto gen = std::bind(std::ref(normal), std::ref(engine));

    lookup_test(state, d, gen);
}
BENCHMARK(goog_lookup_with_many_misses)
BENCH_SIZES;
#endif

// TODO(improve) - this hits us hard
static void dict_lookup_with_heavy_clustering(benchmark::State& state) {
    const std::size_t test_size = state.range(0);
    auto d = build_map<io::dict<std::size_t, std::size_t>>(test_size, inc_gen());

    std::uniform_int_distribution<std::size_t> normal(0, 16 * test_size - 1);
    std::mt19937 engine;
    auto gen = std::bind(std::ref(normal), std::ref(engine));

    lookup_test(state, d, gen);
}
BENCHMARK(dict_lookup_with_heavy_clustering)
BENCH_SIZES;

static void umap_lookup_with_heavy_clustering(benchmark::State& state) {
    const std::size_t test_size = state.range(0);
    auto d = build_map<std::unordered_map<std::size_t, std::size_t>>(test_size, inc_gen());

    std::uniform_int_distribution<std::size_t> normal(0, 16 * test_size - 1);
    std::mt19937 engine;
    auto gen = std::bind(std::ref(normal), std::ref(engine));

    lookup_test(state, d, gen);
}
BENCHMARK(umap_lookup_with_heavy_clustering)
BENCH_SIZES;

#ifdef WITH_GOOGLE_BENCH
static void goog_lookup_with_heavy_clustering(benchmark::State& state) {
    const std::size_t test_size = state.range(0);
    auto d = build_map_google<google::dense_hash_map<std::size_t, std::size_t>>(test_size,
                                                                inc_gen());

    std::uniform_int_distribution<std::size_t> normal(0, 16 * test_size - 1);
    std::mt19937 engine;
    auto gen = std::bind(std::ref(normal), std::ref(engine));

    lookup_test(state, d, gen);
}
BENCHMARK(goog_lookup_with_heavy_clustering)
BENCH_SIZES;
#endif

static void dict_lookup_with_only_collisions(benchmark::State& state) {
    const std::size_t test_size = state.range(0);
    std::uniform_int_distribution<std::size_t> normal(0, test_size - 1);
    std::mt19937 engine;
    auto gen = std::bind(std::ref(normal), std::ref(engine));
    auto d =
        build_map<io::dict<std::size_t, std::size_t, collision_hasher>>(16 * test_size, gen);

    lookup_test(state, d, gen);
}
BENCHMARK(dict_lookup_with_only_collisions)
COLLISION_BENCH_SIZES;

static void umap_lookup_with_only_collisions(benchmark::State& state) {
    const std::size_t test_size = state.range(0);
    std::uniform_int_distribution<std::size_t> normal(0, test_size - 1);
    std::mt19937 engine;
    auto gen = std::bind(std::ref(normal), std::ref(engine));
    auto d = build_map<std::unordered_map<std::size_t, std::size_t, collision_hasher>>(
        16 * test_size, gen);

    lookup_test(state, d, gen);
}
BENCHMARK(umap_lookup_with_only_collisions)
COLLISION_BENCH_SIZES;

#ifdef WITH_GOOGLE_BENCH
static void goog_lookup_with_only_collisions(benchmark::State& state) {
    const std::size_t test_size = state.range(0);
    std::uniform_int_distribution<std::size_t> normal(0, test_size - 1);
    std::mt19937 engine;
    auto gen = std::bind(std::ref(normal), std::ref(engine));
    auto d =
        build_map_google<google::dense_hash_map<std::size_t, std::size_t, collision_hasher>>(
            16 * test_size, gen);

    lookup_test(state, d, gen);
}
BENCHMARK(goog_lookup_with_only_collisions)
COLLISION_BENCH_SIZES;
#endif

const std::size_t build_test_size = 1000;

static void dict_build(benchmark::State& state) {
    for (auto __attribute__((unused)) _ : state) {
        benchmark::DoNotOptimize(
            build_map<io::dict<std::size_t, std::size_t>>(build_test_size, inc_gen()));
    }
}
BENCHMARK(dict_build);

static void umap_build(benchmark::State& state) {
    for (auto __attribute__((unused)) _ : state) {
        benchmark::DoNotOptimize(build_map<std::unordered_map<std::size_t, std::size_t>>(
            build_test_size, inc_gen()));
    }
}
BENCHMARK(umap_build);

#ifdef WITH_GOOGLE_BENCH
static void goog_build(benchmark::State& state) {
    for (auto __attribute__((unused)) _ : state) {
        // benchmark::DoNotOptimize(
        build_map_google<google::dense_hash_map<std::size_t, std::size_t>>(build_test_size,
                                                           inc_gen());
        // );
    }
}
BENCHMARK(goog_build);
#endif

static void dict_build_with_reserve(benchmark::State& state) {
    for (auto __attribute__((unused)) _ : state) {
        benchmark::DoNotOptimize(
            build_map_with_reserve<io::dict<std::size_t, std::size_t>>(build_test_size));
    }
}
BENCHMARK(dict_build_with_reserve);

static void umap_build_with_reserve(benchmark::State& state) {
    for (auto __attribute__((unused)) _ : state) {
        benchmark::DoNotOptimize(
            build_map_with_reserve<std::unordered_map<std::size_t, std::size_t>>(
                build_test_size));
    }
}
BENCHMARK(umap_build_with_reserve);

#ifdef WITH_GOOGLE_BENCH
static void goog_build_with_reserve(benchmark::State& state) {
    for (auto __attribute__((unused)) _ : state) {
        // benchmark::DoNotOptimize(
        build_map_google<google::dense_hash_map<std::size_t, std::size_t>>(build_test_size,
                                                           inc_gen());
        // );
    }
}
BENCHMARK(goog_build_with_reserve);
#endif

template <typename Map>
Map build_string_map(std::size_t size) {
    Map d;

    for (std::size_t i = 0; i != size; ++i) {
        d[std::to_string(i) + std::to_string(i) + std::to_string(i) +
          std::to_string(i) + std::to_string(i) + std::to_string(i) +
          std::to_string(i)] = i;
    }

    return d;
}

#ifdef WITH_GOOGLE_BENCH
template <typename Map>
Map build_string_map_google(std::size_t size) {
    Map d;
    d.set_empty_key("");

    for (std::size_t i = 0; i != size; ++i) {
        d[std::to_string(i) + std::to_string(i) + std::to_string(i) +
          std::to_string(i) + std::to_string(i) + std::to_string(i) +
          std::to_string(i)] = i;
    }

    return d;
}
#endif

static void dict_build_string_keys(benchmark::State& state) {
    for (auto __attribute__((unused)) _ : state) {
        benchmark::DoNotOptimize(
            build_string_map<io::dict<std::string, std::size_t>>(build_test_size));
    }
}
BENCHMARK(dict_build_string_keys);

static void umap_build_string_keys(benchmark::State& state) {
    for (auto __attribute__((unused)) _ : state) {
        benchmark::DoNotOptimize(
            build_string_map<std::unordered_map<std::string, std::size_t>>(
                build_test_size));
    }
}
BENCHMARK(umap_build_string_keys);

#ifdef WITH_GOOGLE_BENCH
static void goog_build_string_keys(benchmark::State& state) {
    for (auto __attribute__((unused)) _ : state) {
        // benchmark::DoNotOptimize(
        build_string_map_google<google::dense_hash_map<std::string, std::size_t>>(
            build_test_size);
        // );
    }
}
BENCHMARK(goog_build_string_keys);
#endif

template <typename Map, typename LookUpKeys>
std::size_t string_lookup_test(Map& map, const LookUpKeys& keys) {
    std::size_t res = 0;
    for (auto&& k : keys) {
        res += map.find(k)->second;
    }

    return res;
}

const std::size_t string_lookup_test_size = 1000;

static void dict_string_lookup(benchmark::State& state) {
    auto d =
        build_string_map<io::dict<std::string, std::size_t>>(string_lookup_test_size);
    std::vector<std::string> keys{ "1111111", "2222222", "3333333",
                                   "4444444", "5555555", "6666666",
                                   "7777777", "8888888", "9999999" };

    for (auto __attribute__((unused)) _ : state) {
        benchmark::DoNotOptimize(string_lookup_test(d, keys));
    }
}
BENCHMARK(dict_string_lookup);

static void umap_string_lookup(benchmark::State& state) {
    auto d = build_string_map<std::unordered_map<std::string, std::size_t>>(
        string_lookup_test_size);
    std::vector<std::string> keys{ "1111111", "2222222", "3333333",
                                   "4444444", "5555555", "6666666",
                                   "7777777", "8888888", "9999999" };

    for (auto __attribute__((unused)) _ : state) {
        benchmark::DoNotOptimize(string_lookup_test(d, keys));
    }
}
BENCHMARK(umap_string_lookup);

static void map_string_lookup(benchmark::State& state) {
    auto d = build_string_map<std::map<std::string, std::size_t>>(
        string_lookup_test_size);
    std::vector<std::string> keys{ "1111111", "2222222", "3333333",
                                   "4444444", "5555555", "6666666",
                                   "7777777", "8888888", "9999999" };

    for (auto __attribute__((unused)) _ : state) {
        benchmark::DoNotOptimize(string_lookup_test(d, keys));
    }
}
BENCHMARK(map_string_lookup);

#ifdef WITH_GOOGLE_BENCH
static void goog_string_lookup(benchmark::State& state) {
    auto d = build_string_map_google<google::dense_hash_map<std::string, std::size_t>>(
        string_lookup_test_size);
    std::vector<std::string> keys{ "1111111", "2222222", "3333333",
                                   "4444444", "5555555", "6666666",
                                   "7777777", "8888888", "9999999" };

    for (auto __attribute__((unused)) _ : state) {
        benchmark::DoNotOptimize(string_lookup_test(d, keys));
    }
}
BENCHMARK(goog_string_lookup);
#endif

int main(int argc, char** argv) {
    benchmark::Initialize(&argc, argv);
    benchmark::RunSpecifiedBenchmarks();
}
