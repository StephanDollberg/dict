#include "catch/single_include/catch.hpp"

#include "../include/dict/dict.hpp"

#include <vector>
#include <string>
#include <future>

struct fake_hasher {
    std::size_t operator()(int) const { return 42; }
};

TEST_CASE("dict insert", "[dict][insert]") {
    SECTION("simple insert") {
        boost::dict<int, std::string> d;
        CHECK(d.size() == 0);

        std::string test_string = "hello";

        d[2345] = test_string;

        CHECK(d[2345] == test_string);
        CHECK(d.size() == 1);
    }

    SECTION("insert collision") {
        boost::dict<int, std::string, fake_hasher> d;

        std::string test_string = "hello";
        std::string test_string2 = "hello2";

        d[1] = test_string;
        CHECK(d[1] == test_string);
        d[2] = test_string2;
        CHECK(d[2] == test_string2);
    }

    SECTION("insert overwrite") {
        boost::dict<int, std::string> d;

        std::string test_string = "hello";
        d[2345] = test_string;
        CHECK(d[2345] == test_string);

        std::string test_string2 = "hello2";
        d[2345] = test_string2;
        CHECK(d[2345] == test_string2);
    }
}

TEST_CASE("dict lookup", "[dict][lookup]") {}

TEST_CASE("dict exists", "[dict][exists]") {}

TEST_CASE("dict delete", "[dict][delete]") {
    SECTION("simple delete") {
        boost::dict<int, std::string> d;

        std::string test_string = "hello";
        d[2345] = test_string;
        CHECK(d[2345] == test_string);

        CHECK(d.erase(2345) == 1);

        CHECK(d[2345] == "");

        CHECK(d.erase(2345) == 1);
        CHECK(d.erase(2345) == 0);
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

TEST_CASE("dict stress", "[dict][stress]") {
    boost::dict<int, int> d;
    CHECK(d.size() == 0);

    for(int i = 0; i != 1000; ++i) {
        d[i] = i;
    }

    for(int i = 0; i != 1000; ++i) {
        CHECK(d[i] == i);
    }
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

        for(auto&& e: d) {
            (void) e;
            CHECK(false);
        }
    }

    SECTION("non-const iterator") {
        boost::dict<int, int> d;
        d[1] = 42;

        for(auto&& e: d) {
            CHECK(e == 42);
        }
    }

    SECTION("const iteration") {
        boost::dict<int, int> d;
        d[1] = 42;

        const auto& const_d = d;

        for(const auto& e: const_d) {
            CHECK(e == 42);
        }
    }

    SECTION("cbegin/cend iteration") {
        boost::dict<int, int> d;
        d[1] = 42;

        for(auto iter = d.cbegin(); iter != d.cend(); ++iter) {
            CHECK(*iter == 42);
        }
    }

    SECTION("skip iterator") {
        // relies on identity hashing for integers
        boost::dict<int, int> d;
        d[1] = 1;
        d[3] = 3;
        d[6] = 6;

        std::vector<int> results;
        for(auto&& e: d) {
            results.push_back(e);
        }

        CHECK(results == std::vector<int>({1, 3, 6}));
    }
}
