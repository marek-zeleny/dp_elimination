#include <catch2/catch_test_macros.hpp>
#include "algorithms/heuristics.hpp"

#include "data_structures/sylvan_zdd_cnf.hpp"

TEST_CASE("SimpleHeuristic functionality", "[heuristics]") {
    using namespace dp;

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
        heuristics::SimpleHeuristic heuristic;

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
        heuristics::SimpleHeuristic heuristic;

        HeuristicResult result = heuristic(empty);
        CHECK_FALSE(result.success);

        result = heuristic(contains_empty);
        CHECK_FALSE(result.success);
    }
}

TEST_CASE("UnitLiteralHeuristic functionality", "[heuristics]") {
    using namespace dp;

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
        heuristics::UnitLiteralHeuristic heuristic;

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
        heuristics::UnitLiteralHeuristic heuristic;

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
        heuristics::UnitLiteralHeuristic heuristic;

        HeuristicResult result = heuristic(cnf);
        CHECK_FALSE(result.success);
    }
}

TEST_CASE("ClearLiteralHeuristic functionality", "[heuristics]") {
    using namespace dp;

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
        heuristics::ClearLiteralHeuristic heuristic;

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
        heuristics::ClearLiteralHeuristic heuristic;

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
        heuristics::ClearLiteralHeuristic heuristic;

        HeuristicResult result = heuristic(cnf);
        CHECK_FALSE(result.success);
    }
}

TEST_CASE("OrderHeuristic functionality", "[heuristics]") {
    using namespace dp;

    SECTION("Correctly finds first variable") {
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

        SECTION("Ascending") {
            heuristics::OrderHeuristic<true> heuristic{0, 1000};

            HeuristicResult result = heuristic(cnf1);
            CHECK(result.success);
            CHECK(result.literal == 1);

            result = heuristic(cnf2);
            CHECK(result.success);
            CHECK(result.literal == 1);
        }

        SECTION("Descending") {
            heuristics::OrderHeuristic<false> heuristic{0, 1000};

            HeuristicResult result = heuristic(cnf1);
            CHECK(result.success);
            CHECK(result.literal == 4);

            result = heuristic(cnf2);
            CHECK(result.success);
            CHECK(result.literal == 4);
        }
    }

    SECTION("Correctly skips missing variable") {
        SylvanZddCnf cnf = SylvanZddCnf::from_vector({
            {2, 3},
            {-2, -4},
            {2, -4},
        });

        SECTION("Ascending") {
            heuristics::OrderHeuristic<true> heuristic{0, 1000};

            HeuristicResult result = heuristic(cnf);
            CHECK(result.success);
            CHECK(result.literal == 2);
        }

        SECTION("Descending") {
            heuristics::OrderHeuristic<false> heuristic{0, 1000};

            HeuristicResult result = heuristic(cnf);
            CHECK(result.success);
            CHECK(result.literal == 4);
        }
    }

    SECTION("Fails for empty and zero formulas") {
        SylvanZddCnf empty;
        SylvanZddCnf contains_empty = SylvanZddCnf::from_vector({{}});

        SECTION("Ascending") {
            heuristics::OrderHeuristic<true> heuristic{0, 1000};

            HeuristicResult result = heuristic(empty);
            CHECK_FALSE(result.success);
        }

        SECTION("Descending") {
            heuristics::OrderHeuristic<false> heuristic{0, 1000};

            HeuristicResult result = heuristic(contains_empty);
            CHECK_FALSE(result.success);
        }
    }

    SECTION("Correctly handles variable range") {
        SylvanZddCnf cnf = SylvanZddCnf::from_vector({
            {-1, -2},
            {2, 3, 4},
            {3, -4},
            {-3, -4},
            {3},
        });

        SECTION("Ascending") {
            heuristics::OrderHeuristic<true> heuristic1{1, 4};
            HeuristicResult result = heuristic1(cnf);
            CHECK(result.success);
            CHECK(result.literal == 1);

            heuristics::OrderHeuristic<true> heuristic2{2, 4};
            result = heuristic2(cnf);
            CHECK(result.success);
            CHECK(result.literal == 2);

            heuristics::OrderHeuristic<true> heuristic3{3, 4};
            result = heuristic3(cnf);
            CHECK(result.success);
            CHECK(result.literal == 3);

            heuristics::OrderHeuristic<true> heuristic4{4, 4};
            result = heuristic4(cnf);
            CHECK(result.success);
            CHECK(result.literal == 4);

            heuristics::OrderHeuristic<true> heuristic5{5, 1000};
            result = heuristic5(cnf);
            CHECK_FALSE(result.success);
        }

        SECTION("Descending") {
            heuristics::OrderHeuristic<false> heuristic1{1, 4};
            HeuristicResult result = heuristic1(cnf);
            CHECK(result.success);
            CHECK(result.literal == 4);

            heuristics::OrderHeuristic<false> heuristic2{1, 3};
            result = heuristic2(cnf);
            CHECK(result.success);
            CHECK(result.literal == 3);

            heuristics::OrderHeuristic<false> heuristic3{1, 2};
            result = heuristic3(cnf);
            CHECK(result.success);
            CHECK(result.literal == 2);

            heuristics::OrderHeuristic<false> heuristic4{1, 1};
            result = heuristic4(cnf);
            CHECK(result.success);
            CHECK(result.literal == 1);

            heuristics::OrderHeuristic<false> heuristic5{5, 1000};
            result = heuristic5(cnf);
            CHECK_FALSE(result.success);
        }
    }
}

TEST_CASE("MinimalScoreHeuristic functionality", "[heuristics]") {
    using namespace dp;

    constexpr auto test_score = [](const SylvanZddCnf::VariableStats &stats) {
        return stats.positive_clause_count + stats.negative_clause_count;
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
        heuristics::MinimalScoreHeuristic<test_score> heuristic{0, 1000};

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
        heuristics::MinimalScoreHeuristic<test_score> heuristic{0, 1000};

        HeuristicResult result = heuristic(cnf);
        CHECK(result.success);
        CHECK(result.literal == 3);
        CHECK(result.score == 1);
    }

    SECTION("Fails for empty and zero formulas") {
        SylvanZddCnf empty;
        SylvanZddCnf contains_empty = SylvanZddCnf::from_vector({{}});
        heuristics::MinimalScoreHeuristic<test_score> heuristic{0, 1000};

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

        heuristics::MinimalScoreHeuristic<test_score> heuristic1{1, 4};
        HeuristicResult result = heuristic1(cnf);
        CHECK(result.success);
        CHECK(result.literal == 1);
        CHECK(result.score == 1);

        heuristics::MinimalScoreHeuristic<test_score> heuristic2{2, 4};
        result = heuristic2(cnf);
        CHECK(result.success);
        CHECK(result.literal == 2);
        CHECK(result.score == 2);

        heuristics::MinimalScoreHeuristic<test_score> heuristic3{3, 4};
        result = heuristic3(cnf);
        CHECK(result.success);
        CHECK(result.literal == 4);
        CHECK(result.score == 3);

        heuristics::MinimalScoreHeuristic<test_score> heuristic4{3, 3};
        result = heuristic4(cnf);
        CHECK(result.success);
        CHECK(result.literal == 3);
        CHECK(result.score == 4);

        heuristics::MinimalScoreHeuristic<test_score> heuristic5{5, 1000};
        result = heuristic5(cnf);
        CHECK_FALSE(result.success);
    }

    SECTION("bloat_score correctly counts") {
        HeuristicResult::Score score;

        SylvanZddCnf::VariableStats stats_0_0{0, 0};
        score = heuristics::scores::bloat_score(stats_0_0);
        CHECK(score == 0);

        SylvanZddCnf::VariableStats stats_1_2{1, 2};
        score = heuristics::scores::bloat_score(stats_1_2);
        CHECK(score == -1);

        SylvanZddCnf::VariableStats stats_2_2{2, 2};
        score = heuristics::scores::bloat_score(stats_2_2);
        CHECK(score == 0);

        SylvanZddCnf::VariableStats stats_4_0{4, 0};
        score = heuristics::scores::bloat_score(stats_4_0);
        CHECK(score == -4);

        SylvanZddCnf::VariableStats stats_0_4{0, 4};
        score = heuristics::scores::bloat_score(stats_0_4);
        CHECK(score == -4);

        SylvanZddCnf::VariableStats stats_5_12{5, 12};
        score = heuristics::scores::bloat_score(stats_5_12);
        CHECK(score == 43);
    }
}
