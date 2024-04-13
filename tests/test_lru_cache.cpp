#include <catch2/catch_test_macros.hpp>
#include "data_structures/lru_cache.hpp"

TEST_CASE("LruCache functionality", "[LruCache]") {
    using Cache = dp::LruCache<int, std::string, 2>;
    Cache cache;

    SECTION("Adds and retrieves elements correctly") {
        std::optional<std::string> result;

        cache.add(1, "one");
        CHECK(cache.size() == 1);

        result = cache.try_get(1);
        CHECK(result.has_value());
        CHECK(result.value() == "one");

        cache.add(2, "two");
        CHECK(cache.size() == 2);

        result = cache.try_get(2);
        CHECK(result.has_value());
        CHECK(result.value() == "two");

        result = cache.try_get(1);
        CHECK(result.has_value());
        CHECK(result.value() == "one");
    }

    SECTION("Adding an existing key replaces original value") {
        std::optional<std::string> result;

        cache.add(1, "one");
        cache.add(1, "one new");
        CHECK(cache.size() == 1);

        result = cache.try_get(1);
        CHECK(result.has_value());
        CHECK(result.value() == "one new");
    }

    SECTION("Exceeding capacity removes the least recently used element") {
        std::optional<std::string> result;

        cache.add(1, "one");
        cache.add(2, "two");

        SECTION("Elements are in add order") {
            cache.add(3, "three");
            CHECK(cache.size() == 2);

            result = cache.try_get(1);
            CHECK_FALSE(result.has_value());

            result = cache.try_get(2);
            CHECK(result.has_value());
            CHECK(result.value() == "two");

            result = cache.try_get(3);
            CHECK(result.has_value());
            CHECK(result.value() == "three");
        }

        SECTION("Elements are touched before exceeding capacity") {
            SECTION("Touched by getting") {
                cache.try_get(1);
            }

            SECTION("Touched by setting") {
                cache.add(1, "one new");
            }

            cache.add(3, "three");
            CHECK(cache.size() == 2);

            result = cache.try_get(1);
            CHECK(result.has_value());
            CHECK(result.value().starts_with("one"));

            result = cache.try_get(2);
            CHECK_FALSE(result.has_value());

            result = cache.try_get(3);
            CHECK(result.has_value());
            CHECK(result.value() == "three");
        }
    }
}
