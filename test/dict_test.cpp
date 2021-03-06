#include "catch/single_include/catch.hpp"

#include "../include/dict/dict.hpp"

#include <unordered_map>
#include <vector>
#include <string>
#include <future>
#include <numeric>
#include <memory>

struct fake_hasher {
    std::size_t operator()(int) const { return 42; }
};

struct identity_hasher {
    std::size_t operator()(int x) const { return x; }
};

struct collision_hasher_different_hash {
    std::size_t operator()(int x) const {
        if(x == 1) return 1;
        if(x == 2) return 1ull | (1ull << 60);
        if(x == 3) return 1ull | (1ull << 61);
        return x;
    }
};

struct moved_tester {
    int var;
    bool moved_from;

    moved_tester() = default;
    explicit moved_tester(int x) : var(x), moved_from(false) {}
    moved_tester(const moved_tester& other) : var(other.var), moved_from(false) {}
    moved_tester(moved_tester&& other) {
        var = other.var;
        moved_from = false;
        other.moved_from = true;
    }
    moved_tester& operator=(const moved_tester& other) {
        var = other.var;
        return *this;
    };
    moved_tester& operator=(moved_tester&& other) {
        var = other.var;
        moved_from = false;
        other.moved_from = true;
        return *this;
    }

    bool operator==(const moved_tester& other) const {
        return var == other.var;
    }
};

struct moved_tester_hasher {
    std::size_t operator()(const moved_tester& tester) const {
        return tester.var;
    }
};

TEST_CASE("dict constructor", "[dict][constructor]") {
    SECTION("default") { io::dict<int, std::string> d; }

    SECTION("iterators") {
        std::vector<std::pair<int, int>> v{ { 1, 2 }, { 3, 4 }, { 1, 42 } };

        io::dict<int, int> d(v.begin(), v.end());

        CHECK(d[1] == 2);
        CHECK(d[3] == 4);
    }

    SECTION("init list") {
        io::dict<int, int> d{ { 1, 2 }, { 3, 4 }, { 1, 42 } };

        CHECK(d[1] == 2);
        CHECK(d[3] == 4);
    }

    SECTION("copy ctor") {
        io::dict<int, int> d2{{1,2}, {3,4}};
        io::dict<int, int> d1(d2);

        d1 = d2;

        CHECK(d1 == d2);
    }

    SECTION("move ctor") {
        io::dict<int, int> d2{{1,2}, {3,4}};
        io::dict<int, int> d3{{1,2}, {3,4}};
        io::dict<int, int> d1(std::move(d2));

        CHECK(d1 == d3);
    }

}

TEST_CASE("operator=", "[dict][operator=]") {
    SECTION("copy assign") {
        io::dict<int, int> d2{{1,2}, {3,4}};
        io::dict<int, int> d1;

        d1 = d2;

        CHECK(d1 == d2);
    }

    SECTION("move assign") {
        io::dict<int, int> d1;
        io::dict<int, int> d2{{1,2}, {3,4}};
        io::dict<int, int> d3{{1,2}, {3,4}};

        d1 = std::move(d2);

        CHECK(d1 == d3);
    }

    SECTION("init list") {
        io::dict<int, int> d1{{42, 42}};
        d1 = {{1,2}, {3,4}};

        io::dict<int, int> d2{{1,2}, {3,4}};

        CHECK(d1 == d2);
    }
}

TEST_CASE("dict operator[]", "[dict][operator[]]") {
    SECTION("simple operator[]") {
        io::dict<int, std::string> d;
        CHECK(d.size() == 0);

        std::string test_string = "hello";

        d[2345] = test_string;

        CHECK(d[2345] == test_string);
        CHECK(d.size() == 1);
    }

    SECTION("operator[] collision") {
        io::dict<int, std::string, fake_hasher> d;

        std::string test_string = "hello";
        std::string test_string2 = "hello2";

        d[1] = test_string;
        d[2] = test_string2;
        CHECK(d[1] == test_string);
        CHECK(d[2] == test_string2);
    }

    SECTION("operator[] collision with different hash") {
        io::dict<int, int, collision_hasher_different_hash> d;

        d[1] = 1;
        d[2] = 2;
        CHECK(d[1] == 1);
        CHECK(d[2] == 2);
    }

    SECTION("operator[] overwrite") {
        io::dict<int, std::string> d;

        std::string test_string = "hello";
        d[2345] = test_string;
        CHECK(d[2345] == test_string);

        std::string test_string2 = "hello2";
        d[2345] = test_string2;
        CHECK(d[2345] == test_string2);
    }
}

TEST_CASE("dict insert", "[dict][insert]") {
    SECTION("insert(value_type)") {
        io::dict<int, int> d;
        auto pair = std::make_pair(1, 2);
        auto res_success = d.insert(pair);
        CHECK(res_success.first->first == 1);
        CHECK(res_success.first->second == 2);
        CHECK(res_success.second == true);
        CHECK(d[1] == 2);

        auto res_fail = d.insert(pair);
        CHECK(res_fail.first->first == 1);
        CHECK(res_fail.first->second == 2);
        CHECK(res_fail.second == false);
    }

    SECTION("insert(value_type&&)") {
        io::dict<int, int> d;
        auto res_success = d.insert(std::make_pair(1, 2));
        CHECK(res_success.first->first == 1);
        CHECK(res_success.first->second == 2);
        CHECK(res_success.second == true);
        CHECK(d[1] == 2);

        auto res_fail = d.insert(std::make_pair(1, 2));
        CHECK(res_fail.first->first == 1);
        CHECK(res_fail.first->second == 2);
        CHECK(res_fail.second == false);
    }

    SECTION("hinted insert(value_type)") {
        // TODO
    }

    SECTION("hinted insert(value_type&&)") {
        // TODO
    }

    SECTION("insert(iter, iter)") {
        std::vector<std::pair<int, int>> v{ { 1, 2 }, { 3, 4 } };

        io::dict<int, int> d;
        d.insert(v.begin(), v.end());
        CHECK(d[1] == 2);
        CHECK(d[3] == 4);
    }

    SECTION("init list") {
        io::dict<int, int> d;
        d.insert({ { 1, 2 }, { 3, 4 } });
        CHECK(d[1] == 2);
        CHECK(d[3] == 4);
    }
}

TEST_CASE("dict insert_or_assign", "[dict][insert_or_assign]") {
    SECTION("insert_or_assign(key, mapped)") {
        {
            io::dict<int, int> d;
            int test_key = 1;
            auto res_success = d.insert_or_assign(test_key, 2);

            CHECK(d.size() == 1);
            CHECK(res_success.first->first == 1);
            CHECK(res_success.first->second == 2);
            CHECK(res_success.second == true);

            auto res_fail = d.insert_or_assign(test_key, 3);

            CHECK(d.size() == 1);
            CHECK(res_success.first->first == 1);
            CHECK(res_success.first->second == 3);
            CHECK(res_fail.second == false);
        }

        // moved from
        {
            io::dict<int, moved_tester> d;
            moved_tester first_test(2);
            int test_key = 1;
            auto res_success = d.insert_or_assign(test_key, std::move(first_test));

            CHECK(d.size() == 1);
            CHECK(res_success.first->first == 1);
            CHECK(res_success.first->second.var == 2);
            CHECK(first_test.moved_from == true);
            CHECK(res_success.second == true);

            moved_tester second_test(3);
            auto res_fail = d.insert_or_assign(test_key, std::move(second_test));

            CHECK(d.size() == 1);
            CHECK(res_success.first->first == 1);
            CHECK(res_success.first->second.var == 3);
            CHECK(second_test.moved_from == true);
            CHECK(res_fail.second == false);
        }
    }

    SECTION("insert_or_assign(key&&, args)") {
        io::dict<moved_tester, int, moved_tester_hasher> d;
        moved_tester first_test(1);
        auto res_success = d.insert_or_assign(std::move(first_test), 2);

        CHECK(d.size() == 1);
        CHECK(res_success.first->first.var == 1);
        CHECK(res_success.first->second == 2);
        CHECK(first_test.moved_from == true);
        CHECK(res_success.second == true);

        moved_tester second_test(1);
        auto res_fail = d.insert_or_assign(std::move(second_test), 3);

        CHECK(d.size() == 1);
        CHECK(res_success.first->first.var == 1);
        CHECK(res_success.first->second == 3);
        CHECK(second_test.moved_from == false);
        CHECK(res_fail.second == false);
    }
}

struct big_hash {
    std::size_t operator()(int x) const { return 1000000 + x; }
};

TEST_CASE("dict rehash", "[dict][rehash]") {
    SECTION("insert rehash") {
        io::dict<int, int, big_hash> d;
        d.max_load_factor(1.0);

        int i = 0;
        while (!d.next_is_rehash()) {
            d[i] = i;
            ++i;
        }

        d[i] = i;
        CHECK(d[i] == i);

        int sum = 0; for(auto&& e: d) { sum += e.second; }
        CHECK(sum == i * (i + 1) / 2);
    }

    SECTION("max_load_factor == 1") {
        io::dict<int, int, big_hash> d;
        d.max_load_factor(1.0);

        int i = 0;
        while (!d.next_is_rehash()) {
            d[i] = i;
            ++i;
        }

        d[i] = i;
        CHECK(d[i] == i);

        int sum = 0; for(auto&& e: d) { sum += e.second; }
        CHECK(sum == i * (i + 1) / 2);
    }

    SECTION("max load factor still correct after rehash") {
        io::dict<int, int, big_hash> d;

        CHECK(d.max_load_factor() == 0.6875);

        for (auto i = 0; i < 1000; ++i) {
            d[i] = i;
        }

        CHECK(d.max_load_factor() == 0.6875);
    }
}

struct only_moveable {
    only_moveable() = default;
    only_moveable(const only_moveable&) = delete;
    only_moveable& operator=(const only_moveable&) = delete;
    only_moveable(only_moveable&&) = default;
    only_moveable& operator=(only_moveable&&) = default;
};

TEST_CASE("dict emplace", "[dict][insert]") {
    SECTION("emplace") {
        io::dict<int, int> d;
        auto res_success = d.emplace(1, 2);
        CHECK(res_success.first->first == 1);
        CHECK(res_success.first->second == 2);
        CHECK(res_success.second == true);
        CHECK(d[1] == 2);

        auto res_fail = d.emplace(1, 2);
        CHECK(res_fail.first->first == 1);
        CHECK(res_fail.first->second == 2);
        CHECK(res_fail.second == false);
    }

    SECTION("emplace moveable") {
        io::dict<int, only_moveable> d;
        auto res_success = d.emplace(1, only_moveable());
        CHECK(res_success.first->first == 1);
        CHECK(res_success.second == true);

        auto res_fail = d.emplace(1, only_moveable());
        CHECK(res_fail.first->first == 1);
        CHECK(res_fail.second == false);
    }

    SECTION("emplace hint") {
        io::dict<int, int> d;
        d[1] = 0;
        auto hint = d.find(1);
        auto res = d.emplace_hint(hint, 1, 2);
        CHECK(res->first == 1);
        CHECK(res->second == 0);
    }
}

TEST_CASE("dict try_emplace", "[dict][try_emplace]") {
    SECTION("try_emplace(key, args)") {
        {
            io::dict<int, int> d;
            int test_key = 1;
            auto res_success = d.try_emplace(test_key, 2);

            CHECK(d.size() == 1);
            CHECK(res_success.first->first == 1);
            CHECK(res_success.first->second == 2);
            CHECK(res_success.second == true);

            auto res_fail = d.try_emplace(test_key, 3);

            CHECK(d.size() == 1);
            CHECK(res_success.first->first == 1);
            CHECK(res_success.first->second == 2);
            CHECK(res_fail.second == false);
        }

        // moved from
        {
            io::dict<int, moved_tester> d;
            moved_tester first_test(2);
            int test_key = 1;
            auto res_success = d.try_emplace(test_key, std::move(first_test));

            CHECK(d.size() == 1);
            CHECK(res_success.first->first == 1);
            CHECK(res_success.first->second.var == 2);
            CHECK(first_test.moved_from == true);
            CHECK(res_success.second == true);

            moved_tester second_test(3);
            auto res_fail = d.try_emplace(test_key, std::move(second_test));

            CHECK(d.size() == 1);
            CHECK(res_success.first->first == 1);
            CHECK(res_success.first->second.var == 2);
            CHECK(second_test.moved_from == false);
            CHECK(res_fail.second == false);
        }
    }

    SECTION("try_emplace(key&&, args)") {
        io::dict<moved_tester, int, moved_tester_hasher> d;
        moved_tester first_test(1);
        auto res_success = d.try_emplace(std::move(first_test), 2);

        CHECK(d.size() == 1);
        CHECK(res_success.first->first.var == 1);
        CHECK(res_success.first->second == 2);
        CHECK(first_test.moved_from == true);
        CHECK(res_success.second == true);

        moved_tester second_test(1);
        auto res_fail = d.try_emplace(std::move(second_test), 3);

        CHECK(d.size() == 1);
        CHECK(res_success.first->first.var == 1);
        CHECK(res_success.first->second == 2);
        CHECK(second_test.moved_from == false);
        CHECK(res_fail.second == false);
    }
}

TEST_CASE("dict swap", "[dict][swap]") {
    SECTION("swap member") {
        io::dict<int, int> d;
        d[0] = 42;

        io::dict<int, int> empty_dict;
        d.swap(empty_dict);

        CHECK(d.size() == 0);
        CHECK(empty_dict.size() == 1);
        CHECK(d[0] == 0);
        CHECK(empty_dict[0] == 42);
    }
    SECTION("adl swap") {
        using namespace io;

        io::dict<int, int> d;
        d[0] = 42;

        io::dict<int, int> empty_dict;
        std::swap(d, empty_dict);

        CHECK(d.size() == 0);
        CHECK(empty_dict.size() == 1);
        CHECK(d[0] == 0);
        CHECK(empty_dict[0] == 42);
    }
}

TEST_CASE("dict find", "[dict][find]") {
    SECTION("simple find") {
        io::dict<int, int> d;
        auto fail_find = d.find(0);
        CHECK(fail_find == d.end());

        d[0] = 42;

        auto success_find = d.find(0);
        CHECK(success_find->first == 0);
        CHECK(success_find->second == 42);
    }
}

TEST_CASE("dict at", "[dict][at]") {
    SECTION("at") {
        io::dict<int, int> d;
        d[0] = 1;
        CHECK(d.at(0) == 1);
        CHECK_THROWS_AS(d.at(42), std::out_of_range);
    }
}

TEST_CASE("dict count", "[dict][count]") {
    io::dict<int, int> d;
    CHECK(d.count(0) == 0);

    d[0] = 1;

    CHECK(d.count(0) == 1);
}

TEST_CASE("equal range", "[dict][equal_range]") {
    io::dict<int, int> d;
    d[0] = 1;

    auto range_fail = d.equal_range(42);
    for (auto iter = range_fail.first; iter != range_fail.second; ++iter) {
        CHECK(false);
    }

    auto range_success = d.equal_range(0);
    for (auto iter = range_success.first; iter != range_success.second;
         ++iter) {
        CHECK(iter->first == 0);
        CHECK(iter->second == 1);
    }
}

struct destructor_check {
    destructor_check() = default;
    destructor_check(std::shared_ptr<bool> ptr) : _ptr(ptr) {}

    ~destructor_check() {
        if (_ptr) {
            *_ptr = true;
        }
    }

    std::shared_ptr<bool> _ptr;
};

struct erase_move_hasher {
    std::size_t operator()(int x) const {
        if(x == 1) return 1;
        if(x == 2) return 2;
        if(x == 3) return 1;
        return x;
    }
};

TEST_CASE("dict erase", "[dict][resize]") {
    SECTION("erase(key)") {
        io::dict<int, std::string> d;

        std::string test_string = "hello";
        d[1] = test_string;
        CHECK(d[1] == test_string);

        CHECK(d.erase(1) == 1);

        CHECK(d.size() == 0);
        CHECK(d[1] == "");

        CHECK(d.erase(1) == 1);
        CHECK(d.erase(1) == 0);
    }

        SECTION("basic") {
            io::dict<int, int, identity_hasher> d;

            d[1] = 2;
            d[2] = 3;
            auto iter = d.find(1);

            CHECK(d.erase(iter)->second == 3);
            CHECK(d.size() == 1);
            CHECK(d[1] == 0);
            CHECK(d.size() == 2);
        }
        // implementation testing
        SECTION("proper iterator after reshift") {
            io::dict<int, int, erase_move_hasher> d;

            d[1] = 1;
            d[2] = 2;
            d[3] = 3;
            CHECK(d.size() == 3);
            auto iter = d.find(1);
            CHECK(iter != d.end());

            CHECK(d.erase(iter)->second == 3);
            CHECK(d.size() == 2);
            CHECK(d[1] == 0);
            CHECK(d.size() == 3);
        }

    SECTION("erase check destructor") {
        io::dict<int, destructor_check> d;
        auto ptr = std::make_shared<bool>(false);
        d[0] = destructor_check(ptr);

        d.erase(0);
        CHECK(d.size() == 0);

        CHECK(*ptr == true);
    }
}

TEST_CASE("dict reserve", "[dict][reserve]") {
    io::dict<int, int> d;
    CHECK(d.size() == 0);

    int test_size = 100;
    d.reserve(test_size);

    for(int i = 0; i != test_size - 1; ++i) {
        d[i] = i;
    }

    CHECK_FALSE(d.next_is_rehash());
}

TEST_CASE("dict insert 1000", "[dict][stress]") {
    io::dict<int, int> d;
    CHECK(d.size() == 0);

    for (int i = 0; i != 1000; ++i) {
        d[i] = i;
    }

    int res = 0;
    for (auto&& e : d) {
        res += e.second;
    }
    CHECK(res == 999 * 1000 / 2);
}

TEST_CASE("dict clear", "[dict][clear]") {
    SECTION("clear") {
        io::dict<int, int> d;
        d[1] = 2;

        d.clear();
        CHECK(d.size() == 0);
        auto res = d.find(1);
        CHECK(res == d.end());
    }
    SECTION("insert after clear") {
        io::dict<int, int> d;
        d[1] = 2;

        d.clear();
        CHECK(d.size() == 0);

        d[1] = 42;
        CHECK(d.size() == 1);
        CHECK(d[1] == 42);
    }
}

TEST_CASE("dict iteration", "[dict][iter]") {
    SECTION("empty iteration") {
        io::dict<int, int> d;

        for (auto&& e : d) {
            (void)e;
            CHECK(false);
        }
    }

    SECTION("non-const iterator") {
        io::dict<int, int> d;
        d[1] = 42;

        for (auto&& e : d) {
            CHECK(e.first == 1);
            CHECK(e.second == 42);
        }
    }

    SECTION("const iteration") {
        io::dict<int, int> d;
        d[1] = 42;

        const auto& const_d = d;

        for (const auto& e : const_d) {
            CHECK(e.first == 1);
            CHECK(e.second == 42);
        }
    }

    SECTION("cbegin/cend iteration") {
        io::dict<int, int> d;
        d[1] = 42;

        for (auto iter = d.cbegin(); iter != d.cend(); ++iter) {
            CHECK(iter->first == 1);
            CHECK(iter->second == 42);
        }
    }

    SECTION("skip iterator") {
        io::dict<int, int, identity_hasher> d;
        d[1] = 1;
        d[3] = 3;
        d[6] = 6;

        std::vector<int> keys;
        std::vector<int> values;
        for (auto&& e : d) {
            keys.push_back(e.first);
            values.push_back(e.second);
        }

        CHECK(keys == std::vector<int>({ 1, 3, 6 }));
        CHECK(values == std::vector<int>({ 1, 3, 6 }));
    }

    SECTION("modify") {
        io::dict<int, int> d;
        d[1] = 42;

        for (auto&& e : d) {
            e.second = 21;
        }
        CHECK(d[1] == 21);
    }

    SECTION("const key") {
        io::dict<int, int> d;
        static_assert(
            std::is_same<const int, decltype(d.begin()->first)>::value,
            "no const key");
    }
}

TEST_CASE("get_allocator", "[dict][get_allocator]") {
    io::dict<int, int> d;
    CHECK(d.get_allocator() == (std::allocator<std::pair<const int, int>>()));
}

TEST_CASE("operator==", "[dict][operator==]") {
    {
        io::dict<int, int> d1;
        io::dict<int, int> d2;
        CHECK(d1 == d2);
    }
    {
        io::dict<int, int> d1{{1,2}, {3,4}};
        io::dict<int, int> d2{{1,2}, {3,4}};
        CHECK(d1 == d2);
    }
    {
        io::dict<int, int> d1{{1,2}, {3,4}};
        io::dict<int, int> d2{{1,2}, {3,5}};
        CHECK(d1 != d2);
    }
}

TEST_CASE("proper iterator after insert", "[dict][insert]") {
    // this test relies on & quadratic size hashing
    io::dict<int, int> d;

    int counter = 9;
    while(!d.next_is_rehash()) {
        d[counter] = counter;
        ++counter;
    }

    // before rehash 24 will map to 8 after rehash to 24
    auto res = d.insert({24, 24});

    CHECK(res.first->first == 24);
    CHECK(res.first->second == 24);
    CHECK(res.second == true);
}

TEST_CASE("hash mixer", "[dict]") {
    {
        std::hash<int> my_hasher;
        std::unordered_map<int, int, io::murmur_hash_mixer<std::hash<int>>> d(10, my_hasher);

        d[1] = 1;
        CHECK(d[1] == 1);
    }

    {
        io::murmur_hash_mixer<std::hash<int>> my_wrapped_hasher;
        std::unordered_map<int, int, io::murmur_hash_mixer<std::hash<int>>> d(10, my_wrapped_hasher);

        d[1] = 1;
        CHECK(d[1] == 1);
    }

    {
        io::dict<int, int, io::murmur_hash_mixer<std::hash<int>>> d_with_mixer;
        io::dict<int, int> d_without_mixer;

        d_with_mixer[0] = 0;
        d_with_mixer[1] = 1;
        d_with_mixer[2] = 2;
        CHECK(d_with_mixer[1] == 1);

        d_without_mixer[0] = 0;
        d_without_mixer[1] = 1;
        d_without_mixer[2] = 2;
        CHECK(d_without_mixer[1] == 1);

        std::vector<int> ordered{ 0, 1, 2 };

        CHECK(std::equal(ordered.begin(), ordered.end(),
                         d_without_mixer.begin(),
                         [](int lhs, std::pair<int, int> rhs) -> bool {
                             return lhs == rhs.first;
                         }));
        CHECK(!std::equal(ordered.begin(), ordered.end(), d_with_mixer.begin(),
                          [](int lhs, std::pair<int, int> rhs) -> bool {
                              return lhs == rhs.first;
                          }));
    }
}

#ifdef __cpp_deduction_guides
TEST_CASE("C++17 deduction guides", "[dict][C++17]") {
    io::dict<int, int> d_with_types{{1,2}, {3,4}};
    io::dict d_without_types(d_with_types.begin(), d_with_types.end());

    CHECK(d_without_types[1] == 2);
    CHECK(d_without_types[3] == 4);
}
#endif

TEST_CASE("nothrow move constructalbe", "[dict][construct]") {
    // default ctors
    static_assert(std::is_default_constructible<io::dict<int, std::string>>::value,
        "dict should be default constructible");
    // Need to make dict() not allocate for that
    // static_assert(std::is_nothrow_default_constructible<io::dict<int, std::string>>::value,
    //     "dict should be noexcept default constructible");

    // copy ctors
    static_assert(std::is_copy_constructible<io::dict<int, std::string>>::value,
        "dict should be copy constructible");

    // move ctors
    static_assert(std::is_move_constructible<io::dict<int, std::string>>::value,
        "dict should be move constructible");
    static_assert(std::is_nothrow_move_constructible<io::dict<int, std::string>>::value,
        "dict should be noexcept move constructible");

    // copy assignment
    static_assert(std::is_copy_assignable<io::dict<int, std::string>>::value,
        "dict should be copy assignable");

    // move assignment
    static_assert(std::is_move_assignable<io::dict<int, std::string>>::value,
        "dict should be move assignable");
    static_assert(std::is_nothrow_move_assignable<io::dict<int, std::string>>::value,
        "dict should be noexcept move assignable");

#ifdef __cpp_lib_is_swappable
    // swapability
    static_assert(std::is_swappable<io::dict<int, std::string>>::value,
        "dict should be swappable");
    // need to figure this out
    // static_assert(std::is_nothrow_swappable<io::dict<int, std::string>>::value,
    //     "dict should be noexcept swappable");
#endif
}

#if __cpp_lib_memory_resource
TEST_CASE("pmr support", "[dict][pmr]") {
    auto allocator = std::experimental::pmr::new_delete_resource();
    io::pmr::dict<int, int> pmr_dict(allocator);

    pmr_dict[1] = 2;

    CHECK(pmr_dict[1] == 2);
}
#endif
