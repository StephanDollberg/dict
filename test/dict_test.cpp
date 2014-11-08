#include "catch/single_include/catch.hpp"

#include "../include/dict/dict.hpp"

#include <vector>
#include <string>
#include <future>
#include <numeric>
#include <memory>


TEST_CASE("dict constructor", "[dict][constructor]") {
    SECTION("default") {
        boost::dict<int, std::string> d;
    }

    SECTION("iterators") {
        std::vector<std::pair<int, int>> v{{1, 2}, {3, 4}, {1, 42}};

        boost::dict<int, int> d(v.begin(), v.end());

        CHECK(d[1] == 2);
        CHECK(d[3] == 4);
    }

    SECTION("init list") {
        boost::dict<int, int> d{{1, 2}, {3, 4}, {1, 42}};

        CHECK(d[1] == 2);
        CHECK(d[3] == 4);
    }
}

struct fake_hasher {
    std::size_t operator()(int) const { return 42; }
};

TEST_CASE("dict operator[]", "[dict][operator[]]") {
    SECTION("simple operator[]") {
        boost::dict<int, std::string> d;
        CHECK(d.size() == 0);

        std::string test_string = "hello";

        d[2345] = test_string;

        CHECK(d[2345] == test_string);
        CHECK(d.size() == 1);
    }

    SECTION("operator[] collision") {
        boost::dict<int, std::string, fake_hasher> d;

        std::string test_string = "hello";
        std::string test_string2 = "hello2";

        d[1] = test_string;
        CHECK(d[1] == test_string);
        d[2] = test_string2;
        CHECK(d[2] == test_string2);
    }

    SECTION("operator[] overwrite") {
        boost::dict<int, std::string> d;

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
        boost::dict<int, int> d;
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
}

struct big_hash {
    std::size_t operator()(int x) const {
        return 1000000 + x;
    }
};

TEST_CASE("dict rehash", "[dict][rehash]") {
    SECTION("insert rehash") {
        boost::dict<int, int, big_hash> d;
        d[1] = 42;

        int i = 0;
        while(!d.next_is_rehash()) {
            d[i] = i;
            ++i;
        }

        d[42] = 42;
        CHECK(d[42] == 42);

    }

    SECTION("max_load_factor == 1") {
        boost::dict<int, int, big_hash> d;
        d.max_load_factor(1.0);

        // 11 is starting table size
        for(int i = 0; i != 11; ++i) {
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
        boost::dict<int, int> d;
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
        boost::dict<int, only_moveable> d;
        auto res_success = d.emplace(1, only_moveable());
        CHECK(res_success.first->first == 1);
        CHECK(res_success.second == true);

        auto res_fail = d.emplace(1, only_moveable());
        CHECK(res_fail.first->first == 1);
        CHECK(res_fail.second == false);
    }

    SECTION("emplace hint") {
        boost::dict<int, int> d;
        d[1] = 0;
        auto hint = d.find(1);
        auto res_fail = d.emplace_hint(hint, 1, 2);
        CHECK(res_fail.first->first == 1);
        CHECK(res_fail.first->second == 0);
        CHECK(res_fail.second == false);
    }
}

TEST_CASE("dict find", "[dict][find]") {
    SECTION("simple find") {
        boost::dict<int, int> d;
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
        boost::dict<int, int> d;
        d[0] = 1;
        CHECK(d.at(0) == 1);
        CHECK_THROWS_AS(d.at(42), std::out_of_range);
    }
}

TEST_CASE("dict count", "[dict][count]") {
        boost::dict<int, int> d;
        CHECK(d.count(0) == 0);

        d[0] = 1;

        CHECK(d.count(0) == 1);
}

TEST_CASE("equal range", "[dict][equal_range]") {
    boost::dict<int, int> d;
    d[0] = 1;

    auto range_fail = d.equal_range(42);
    for(auto iter = range_fail.first; iter != range_fail.second; ++iter) {
        CHECK(false);
    }

    auto range_success = d.equal_range(0);
    for(auto iter = range_success.first; iter != range_success.second; ++iter) {
        CHECK(iter->first == 0);
        CHECK(iter->second == 1);
    }
}

TEST_CASE("dict exists", "[dict][exists]") {}

struct destructor_check {
    destructor_check() = default;
    destructor_check(std::shared_ptr<bool> ptr) : _ptr(ptr) {}

    ~destructor_check() {
        if(_ptr) {
            *_ptr = true;
        }
    }

    std::shared_ptr<bool> _ptr;
};

TEST_CASE("dict erase", "[dict][erase]") {
    SECTION("simple erase") {
        boost::dict<int, std::string> d;

        std::string test_string = "hello";
        d[2345] = test_string;
        CHECK(d[2345] == test_string);

        CHECK(d.erase(2345) == 1);

        CHECK(d[2345] == "");

        CHECK(d.erase(2345) == 1);
        CHECK(d.erase(2345) == 0);
    }

    SECTION("erase check destructor") {
        boost::dict<int, destructor_check> d;
        auto ptr = std::make_shared<bool>(false);
        d[0] = destructor_check(ptr);

        d.erase(0);

        CHECK(*ptr == true);
    }
}

TEST_CASE("dict resize", "[dict][exists]") {
    boost::dict<int, std::string> d;
    CHECK(d.size() == 0);

    std::string test_string = "hello";

    d[2345] = test_string;

    CHECK(d[2345] == test_string);

    d.reserve(1000);
    CHECK(d.size() == 1);
    CHECK(d[2345] == test_string);
}

TEST_CASE("dict insert 1000", "[dict][stress]") {
    boost::dict<int, int> d;
    CHECK(d.size() == 0);

    for (int i = 0; i != 1000; ++i) {
        d[i] = i;
    }

    int res = 0;
    for(auto&& e: d) {
        res += e.second;
    }
    CHECK(res == 999 * 1000 / 2);
}

TEST_CASE("dict clear", "[dict][clear]") {
    boost::dict<int, int> d;
    d[1] = 2;

    d.clear();
    CHECK(d.size() == 0);
}

TEST_CASE("dict iteration", "[dict][iter]") {
    SECTION("empty iteration") {
        boost::dict<int, int> d;

        for (auto&& e : d) {
            (void)e;
            CHECK(false);
        }
    }

    SECTION("non-const iterator") {
        boost::dict<int, int> d;
        d[1] = 42;

        for (auto&& e : d) {
            CHECK(e.first == 1);
            CHECK(e.second == 42);
        }
    }

    SECTION("const iteration") {
        boost::dict<int, int> d;
        d[1] = 42;

        const auto& const_d = d;

        for (const auto& e : const_d) {
            CHECK(e.first == 1);
            CHECK(e.second == 42);
        }
    }

    SECTION("cbegin/cend iteration") {
        boost::dict<int, int> d;
        d[1] = 42;

        for (auto iter = d.cbegin(); iter != d.cend(); ++iter) {
            CHECK(iter->first == 1);
            CHECK(iter->second == 42);
        }
    }

    SECTION("skip iterator") {
        // relies on identity hashing for integers
        boost::dict<int, int> d;
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
        boost::dict<int, int> d;
        d[1] = 42;

        for (auto&& e : d) {
            e.second = 21;
        }
        CHECK(d[1] == 21);
    }

    SECTION("const key") {
        boost::dict<int, int> d;
        static_assert(
            std::is_same<const int, decltype(d.begin()->first)>::value,
            "no const key");
    }
}
