#include "catch2/catch_test_macros.hpp"
#include "data_structures/lru_cache.hpp"

TEST_CASE("Example unit test.") {
    dp::LruCache<size_t, int, 3> cache;
    cache.add(11, -42);
    REQUIRE(cache.size() == 1);
}
