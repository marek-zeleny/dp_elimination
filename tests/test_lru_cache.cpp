#include <catch2/catch_test_macros.hpp>
#include "data_structures/lru_cache.hpp"

TEST_CASE("LruCache functionality", "[LruCache]") {
    using Cache = dp::LruCache<int, std::string, 2>;
    Cache cache;

    SECTION("Adds and retrieves elements correctly") {
        bool found;
        std::string value;

        cache.add(1, "one");
        CHECK(cache.size() == 1);

        found = cache.try_get(1, value);
        CHECK(found);
        CHECK(value == "one");

        cache.add(2, "two");
        CHECK(cache.size() == 2);

        found = cache.try_get(2, value);
        CHECK(found);
        CHECK(value == "two");

        found = cache.try_get(1, value);
        CHECK(found);
        CHECK(value == "one");
    }

    SECTION("Adding an existing key replaces original value") {
        bool found;
        std::string value;

        cache.add(1, "one");
        cache.add(1, "one new");
        CHECK(cache.size() == 1);

        found = cache.try_get(1, value);
        CHECK(found);
        CHECK(value == "one new");
    }

    SECTION("Exceeding capacity removes the least recently used element") {
        bool found;
        std::string value;

        cache.add(1, "one");
        cache.add(2, "two");

        SECTION("Elements are in add order") {
            cache.add(3, "three");
            CHECK(cache.size() == 2);

            found = cache.try_get(1, value);
            CHECK_FALSE(found);

            found = cache.try_get(2, value);
            CHECK(found);
            CHECK(value == "two");

            found = cache.try_get(3, value);
            CHECK(found);
            CHECK(value == "three");
        }

        SECTION("Elements are touched before exceeding capacity") {
            SECTION("Touched by getting") {
                cache.try_get(1, value);
            }

            SECTION("Touched by setting") {
                cache.add(1, "one new");
            }

            cache.add(3, "three");
            CHECK(cache.size() == 2);

            found = cache.try_get(1, value);
            CHECK(found);
            CHECK(value.starts_with("one"));

            found = cache.try_get(2, value);
            CHECK_FALSE(found);

            found = cache.try_get(3, value);
            CHECK(found);
            CHECK(value == "three");
        }
    }
}
