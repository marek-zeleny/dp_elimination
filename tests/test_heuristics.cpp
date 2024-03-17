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
    constexpr auto test_score = [](const SylvanZddCnf::VariableStats &stats) {
        return static_cast<HeuristicResult::Score>(stats.positive_clause_count + stats.negative_clause_count);
    };

    SECTION("Correctly finds minimum") {
        SylvanZddCnf cnf1 = SylvanZddCnf::from_vector({
            {-1, -2},
            {2, 3, 4},
            {3, -4},
            {-3, -4},
            {4},
        });
        SylvanZddCnf cnf2 = SylvanZddCnf::from_vector({
            {-3, -4},
            {1, 2, 3},
            {-1, 2},
            {-1, -2},
            {1},
        });
        MinimalScoreHeuristic<test_score> heuristic{0, 1000};

        HeuristicResult result = heuristic(cnf1);
        CHECK(result.success);
        CHECK(result.literal == 1);
        CHECK(result.score == 1);

        result = heuristic(cnf2);
        CHECK(result.success);
        CHECK(result.literal == 4);
        CHECK(result.score == 1);
    }

    SECTION("Correctly skips missing variable") {
        SylvanZddCnf cnf = SylvanZddCnf::from_vector({
            {1, 3},
            {-1, -4},
            {1, -4},
        });
        MinimalScoreHeuristic<test_score> heuristic{0, 1000};

        HeuristicResult result = heuristic(cnf);
        CHECK(result.success);
        CHECK(result.literal == 3);
        CHECK(result.score == 1);
    }

    SECTION("Fails for empty and zero formulas") {
        SylvanZddCnf empty;
        SylvanZddCnf contains_empty = SylvanZddCnf::from_vector({{}});
        MinimalScoreHeuristic<test_score> heuristic{0, 1000};

        HeuristicResult result = heuristic(empty);
        CHECK_FALSE(result.success);

        result = heuristic(contains_empty);
        CHECK_FALSE(result.success);
    }

    SECTION("Correctly handles variable range") {
        SylvanZddCnf cnf = SylvanZddCnf::from_vector({
            {-1, -2},
            {2, 3, 4},
            {3, -4},
            {-3, -4},
            {3},
        });

        MinimalScoreHeuristic<test_score> heuristic1{1, 4};
        HeuristicResult result = heuristic1(cnf);
        CHECK(result.success);
        CHECK(result.literal == 1);
        CHECK(result.score == 1);

        MinimalScoreHeuristic<test_score> heuristic2{2, 4};
        result = heuristic2(cnf);
        CHECK(result.success);
        CHECK(result.literal == 2);
        CHECK(result.score == 2);

        MinimalScoreHeuristic<test_score> heuristic3{3, 4};
        result = heuristic3(cnf);
        CHECK(result.success);
        CHECK(result.literal == 4);
        CHECK(result.score == 3);

        MinimalScoreHeuristic<test_score> heuristic4{3, 3};
        result = heuristic4(cnf);
        CHECK(result.success);
        CHECK(result.literal == 3);
        CHECK(result.score == 4);

        MinimalScoreHeuristic<test_score> heuristic5{5, 1000};
        result = heuristic5(cnf);
        CHECK_FALSE(result.success);
    }

    SECTION("bloat_score correctly counts") {
        HeuristicResult::Score score;

        SylvanZddCnf::VariableStats stats_0_0{0, 0};
        score = bloat_score(stats_0_0);
        CHECK(score == 0);

        SylvanZddCnf::VariableStats stats_1_2{1, 2};
        score = bloat_score(stats_1_2);
        CHECK(score == -1);

        SylvanZddCnf::VariableStats stats_2_2{2, 2};
        score = bloat_score(stats_2_2);
        CHECK(score == 0);

        SylvanZddCnf::VariableStats stats_4_0{4, 0};
        score = bloat_score(stats_4_0);
        CHECK(score == -4);

        SylvanZddCnf::VariableStats stats_0_4{0, 4};
        score = bloat_score(stats_0_4);
        CHECK(score == -4);

        SylvanZddCnf::VariableStats stats_5_12{5, 12};
        score = bloat_score(stats_5_12);
        CHECK(score == 43);
    }
}
