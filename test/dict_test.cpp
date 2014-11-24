#include "catch/single_include/catch.hpp"

#include "../include/dict/dict.hpp"

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
        CHECK(d[1] == test_string);
        d[2] = test_string2;
        CHECK(d[2] == test_string2);
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

struct big_hash {
    std::size_t operator()(int x) const { return 1000000 + x; }
};

TEST_CASE("dict rehash", "[dict][rehash]") {
    SECTION("insert rehash") {
        io::dict<int, int, big_hash> d;
        d[1] = 42;

        int i = 0;
        while (!d.next_is_rehash()) {
            d[i] = i;
            ++i;
        }

        d[42] = 42;
        CHECK(d[42] == 42);
    }

    SECTION("max_load_factor == 1") {
        io::dict<int, int, big_hash> d;
        d.max_load_factor(1.0);

        // 11 is starting table size
        for (int i = 0; i != 11; ++i) {
            d[i] = i;
        }

        CHECK(d[10] == 10);
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

struct moved_tester {
    int var;
    bool moved_from;

    moved_tester() = default;
    explicit moved_tester(int x) : var(x), moved_from(false) {}
    moved_tester(const moved_tester&) {};
    moved_tester(moved_tester&& other) {
        var = other.var;
        moved_from = false;
        other.moved_from = true;
    }
    moved_tester& operator=(const moved_tester&) {
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

TEST_CASE("dict try_emplace", "[dict][try_emplace]") {
    SECTION("try_emplace(key, args)") {
        {
            io::dict<int, int> d;
            auto res_success = d.try_emplace(1, 2);

            CHECK(d.size() == 1);
            CHECK(res_success.first->first == 1);
            CHECK(res_success.first->second == 2);
            CHECK(res_success.second == true);

            auto res_fail = d.try_emplace(1, 3);

            CHECK(d.size() == 1);
            CHECK(res_success.first->first == 1);
            CHECK(res_success.first->second == 2);
            CHECK(res_fail.second == false);
        }

        // moved from
        {
            io::dict<int, moved_tester> d;
            moved_tester first_test(2);
            auto res_success = d.try_emplace(1, std::move(first_test));

            CHECK(d.size() == 1);
            CHECK(res_success.first->first == 1);
            CHECK(res_success.first->second.var == 2);
            CHECK(first_test.moved_from == true);
            CHECK(res_success.second == true);

            moved_tester second_test(3);
            auto res_fail = d.try_emplace(1, std::move(second_test));

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

TEST_CASE("dict exists", "[dict][exists]") {}

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

TEST_CASE("dict erase", "[dict][erase]") {
    SECTION("erase(key)") {
        io::dict<int, std::string> d;

        std::string test_string = "hello";
        d[1] = test_string;
        CHECK(d[1] == test_string);

        CHECK(d.erase(1) == 1);

        CHECK(d[1] == "");

        CHECK(d.erase(1) == 1);
        CHECK(d.erase(1) == 0);
    }

    SECTION("erase(iterator)") {
        io::dict<int, int, identity_hasher> d;

        d[1] = 2;
        d[2] = 3;
        auto iter = d.find(1);

        CHECK(d.erase(iter)->second == 3);
        CHECK(d[1] == 0);
    }

    SECTION("erase check destructor") {
        io::dict<int, destructor_check> d;
        auto ptr = std::make_shared<bool>(false);
        d[0] = destructor_check(ptr);

        d.erase(0);

        CHECK(*ptr == true);
    }
}

TEST_CASE("dict resize", "[dict][exists]") {
    io::dict<int, std::string> d;
    CHECK(d.size() == 0);

    std::string test_string = "hello";

    d[2345] = test_string;

    CHECK(d[2345] == test_string);

    d.reserve(1000);
    CHECK(d.size() == 1);
    CHECK(d[2345] == test_string);
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
    io::dict<int, int> d;
    d[1] = 2;

    d.clear();
    CHECK(d.size() == 0);
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
