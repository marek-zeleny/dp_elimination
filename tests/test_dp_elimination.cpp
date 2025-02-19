#include <catch2/catch_test_macros.hpp>
#include "algorithms/dp_elimination.hpp"

#include "data_structures/sylvan_zdd_cnf.hpp"

TEST_CASE("SylvanZddCnf eliminate operation", "[dp elimination]") {
    using namespace dp;

    SECTION("Eliminate a variable, resulting in an empty formula") {
        SylvanZddCnf cnf = SylvanZddCnf::from_vector({
            {1,  2},
            {-1, -2},
        });
        auto result = eliminate(cnf, 1);
        CHECK(result.is_empty());
    }

    SECTION("Eliminate a variable, resulting in an empty clause") {
        SylvanZddCnf cnf = SylvanZddCnf::from_vector({
            {1},
            {-1},
        });
        auto result = eliminate(cnf, 1);
        CHECK(result.contains_empty());
    }

    // TODO: Adjust expected result to work without subsumed removal
//    SECTION("Eliminate a variable in a complex formula") {
//        SylvanZddCnf cnf = SylvanZddCnf::from_vector({
//            {1, 2, 3},
//            {2, 4},
//            {1, 3, 4},
//            {2, 5, 6},
//            {-4},
//        });
//        auto result = eliminate(cnf, 4);
//        SylvanZddCnf expected = SylvanZddCnf::from_vector({
//            {2},
//            {1, 3},
//        });
//        CHECK(result == expected);
//    }
}
