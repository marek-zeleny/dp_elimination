#include <catch2/catch_test_macros.hpp>
#include "algorithms/heuristics.hpp"

#include "data_structures/sylvan_zdd_cnf.hpp"

using namespace dp;

TEST_CASE("SimpleHeuristic functionality", "[heuristics]") {
    SECTION("Returns the root (smallest) literal") {
        SylvanZddCnf cnf1 = SylvanZddCnf::from_vector({
            {1,  2},
            {-1, -2, 4},
            {-3},
        });
        SylvanZddCnf cnf2 = SylvanZddCnf::from_vector({
            {-1, -2, 4},
            {2, 3},
        });
        SimpleHeuristic heuristic;

        HeuristicResult result = heuristic(cnf1);
        CHECK(result.success);
        CHECK(result.literal == 1);

        result = heuristic(cnf2);
        CHECK(result.success);
        CHECK(result.literal == -1);
    }

    SECTION("Fails for empty and zero formulas") {
        SylvanZddCnf empty;
        SylvanZddCnf contains_empty = SylvanZddCnf::from_vector({{}});
        SimpleHeuristic heuristic;

        HeuristicResult result = heuristic(empty);
        CHECK_FALSE(result.success);

        result = heuristic(contains_empty);
        CHECK_FALSE(result.success);
    }
}

TEST_CASE("UnitLiteralHeuristic functionality", "[heuristics]") {
    SECTION("Returns a unit literal") {
        SylvanZddCnf cnf1 = SylvanZddCnf::from_vector({
            {1,  2},
            {-1, -2, 4},
            {-3},
        });
        SylvanZddCnf cnf2 = SylvanZddCnf::from_vector({
            {-1, -2, 4},
            {2},
            {-4},
        });
        UnitLiteralHeuristic heuristic;

        HeuristicResult result = heuristic(cnf1);
        CHECK(result.success);
        CHECK(result.literal == -3);

        result = heuristic(cnf2);
        CHECK(result.success);
        CHECK((result.literal == 2 || result.literal == -4));
    }

    SECTION("Fails for empty and zero formulas") {
        SylvanZddCnf empty;
        SylvanZddCnf contains_empty = SylvanZddCnf::from_vector({{}});
        UnitLiteralHeuristic heuristic;

        HeuristicResult result = heuristic(empty);
        CHECK_FALSE(result.success);

        result = heuristic(contains_empty);
        CHECK_FALSE(result.success);
    }

    SECTION("Fails when no unit literal exists") {
        SylvanZddCnf cnf = SylvanZddCnf::from_vector({
            {-1, -2, 4},
            {2, 3},
        });
        UnitLiteralHeuristic heuristic;

        HeuristicResult result = heuristic(cnf);
        CHECK_FALSE(result.success);
    }
}

TEST_CASE("ClearLiteralHeuristic functionality", "[heuristics]") {
    SECTION("Returns a clear literal") {
        SylvanZddCnf cnf1 = SylvanZddCnf::from_vector({
            {1,  2},
            {-1, -2, 4},
            {-3},
        });
        SylvanZddCnf cnf2 = SylvanZddCnf::from_vector({
            {-1, -2, 4},
            {2},
            {-4},
        });
        ClearLiteralHeuristic heuristic;

        HeuristicResult result = heuristic(cnf1);
        CHECK(result.success);
        CHECK((result.literal == -3 || result.literal == 4));

        result = heuristic(cnf2);
        CHECK(result.success);
        CHECK(result.literal == -1);
    }

    SECTION("Fails for empty and zero formulas") {
        SylvanZddCnf empty;
        SylvanZddCnf contains_empty = SylvanZddCnf::from_vector({{}});
        ClearLiteralHeuristic heuristic;

        HeuristicResult result = heuristic(empty);
        CHECK_FALSE(result.success);

        result = heuristic(contains_empty);
        CHECK_FALSE(result.success);
    }

    SECTION("Fails when no clear literal exists") {
        SylvanZddCnf cnf = SylvanZddCnf::from_vector({
            {-1, -2, 4},
            {2, 3},
            {1, -3, -4},
        });
        ClearLiteralHeuristic heuristic;

        HeuristicResult result = heuristic(cnf);
        CHECK_FALSE(result.success);
    }
}

TEST_CASE("MinimalScoreHeuristic functionality", "[heuristics]") {
    SECTION("Bloat heuristic correctly counts score") {
        SylvanZddCnf cnf1 = SylvanZddCnf::from_vector({
            {-1, -2},
            {2, 3},
            {1, 2, -3},
            {-2, -3},
            {3},
        });
        // variable scores:
        // 1: -1
        // 2: 0
        // 3: 0
        SylvanZddCnf cnf2 = SylvanZddCnf::from_vector({
            {-1, -2, 4},
            {2, 3},
            {1, 2, -3, 4},
            {-2},
        });
        // variable scores:
        // 1: -1
        // 2: 0
        // 3: -1
        // 4: -2
        MinimalScoreHeuristic<bloat_score> heuristic{0, 1000};

        HeuristicResult result = heuristic(cnf1);
        CHECK(result.success);
        CHECK(result.literal == 1);
        CHECK(result.score == -1);

        result = heuristic(cnf2);
        CHECK(result.success);
        CHECK(result.literal == 4);
        CHECK(result.score == -2);
    }

    SECTION("Fails for empty and zero formulas") {
        SylvanZddCnf empty;
        SylvanZddCnf contains_empty = SylvanZddCnf::from_vector({{}});
        MinimalScoreHeuristic<bloat_score> heuristic{0, 1000};

        HeuristicResult result = heuristic(empty);
        CHECK_FALSE(result.success);

        result = heuristic(contains_empty);
        CHECK_FALSE(result.success);
    }

    SECTION("Correctly handles variable range") {
        SylvanZddCnf cnf = SylvanZddCnf::from_vector({
            {-1, -2, 4},
            {2, -3},
            {1, 2, -3, 4},
            {-2, 4},
        });
        // variable scores:
        // 1: -1
        // 2: 0
        // 3: -2
        // 4: -3

        MinimalScoreHeuristic<bloat_score> heuristic1{1, 4};
        HeuristicResult result = heuristic1(cnf);
        CHECK(result.success);
        CHECK(result.literal == 4);
        CHECK(result.score == -3);

        MinimalScoreHeuristic<bloat_score> heuristic2{1, 3};
        result = heuristic2(cnf);
        CHECK(result.success);
        CHECK(result.literal == 3);
        CHECK(result.score == -2);

        MinimalScoreHeuristic<bloat_score> heuristic3{1, 2};
        result = heuristic3(cnf);
        CHECK(result.success);
        CHECK(result.literal == 1);
        CHECK(result.score == -1);

        MinimalScoreHeuristic<bloat_score> heuristic4{2, 2};
        result = heuristic4(cnf);
        CHECK(result.success);
        CHECK(result.literal == 2);
        CHECK(result.score == 0);

        MinimalScoreHeuristic<bloat_score> heuristic5{5, 1000};
        result = heuristic5(cnf);
        CHECK_FALSE(result.success);
    }
}
