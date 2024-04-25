#include <catch2/catch_test_macros.hpp>
#include "algorithms/unit_propagation.hpp"

TEST_CASE("is_clause_absorbed tests", "[absorbed clause detection]") {
    namespace without = dp::absorbed_clause_detection::without_conversion;
    namespace with = dp::absorbed_clause_detection::with_conversion;
    using namespace dp;
    
    SECTION("Clause already in formula is absorbed") {
        std::vector<std::vector<int32_t>> clauses {
                {1, 2},
                {-1, 2, 3},
                {2, -3},
        };

        SECTION("With conversion") {
            WatchedLiterals formula = WatchedLiterals::from_vector(clauses);
            CHECK(with::is_clause_absorbed(formula, {1, 2}));

            formula.backtrack_to(0);
            CHECK(with::is_clause_absorbed(formula, {-1, 2, 3}));

            formula.backtrack_to(0);
            CHECK(with::is_clause_absorbed(formula, {2, -3}));
        }
        SECTION("Without conversion") {
            SylvanZddCnf cnf = SylvanZddCnf::from_vector(clauses);
            CHECK(without::is_clause_absorbed(cnf, {1, 2}));
            CHECK(without::is_clause_absorbed(cnf, {-1, 2, 3}));
            CHECK(without::is_clause_absorbed(cnf, {2, -3}));
        }
    }

    SECTION("Simple non-absorbed clause") {
        std::vector<std::vector<int32_t>> clauses = {
                {-1, 2},
                {-2, 3},
        };
        SECTION("With conversion") {
            WatchedLiterals formula = WatchedLiterals::from_vector(clauses);
            CHECK_FALSE(with::is_clause_absorbed(formula, {1, -3}));
        }
        SECTION("Without conversion") {
            SylvanZddCnf cnf = SylvanZddCnf::from_vector(clauses);
            CHECK_FALSE(without::is_clause_absorbed(cnf, {1, -3}));
        }
    }

    SECTION("Simple absorbed clause") {
        std::vector<std::vector<int32_t>> clauses = {
                {-1, 2},
                {-2, 3},
        };
        SECTION("With conversion") {
            WatchedLiterals formula = WatchedLiterals::from_vector(clauses);
            CHECK(with::is_clause_absorbed(formula, {-1, 3}));
        }
        SECTION("Without conversion") {
            SylvanZddCnf cnf = SylvanZddCnf::from_vector(clauses);
            CHECK(without::is_clause_absorbed(cnf, {-1, 3}));
        }
    }

    SECTION("Unit clause is absorbed IFF its literal is unit-deductible from the formula") {
        std::vector<std::vector<int32_t>> clauses = {
                {-1, 2},
                {-2, 3},
                {1},
                {4, 5},
        };
        SECTION("With conversion") {
            WatchedLiterals formula = WatchedLiterals::from_vector(clauses);
            CHECK(with::is_clause_absorbed(formula, {3}));

            formula.backtrack_to(0);
            CHECK_FALSE(with::is_clause_absorbed(formula, {4}));
        }
        SECTION("Without conversion") {
            SylvanZddCnf cnf = SylvanZddCnf::from_vector(clauses);
            CHECK(without::is_clause_absorbed(cnf, {3}));
            CHECK_FALSE(without::is_clause_absorbed(cnf, {4}));
        }
    }

    SECTION("Superclause is always absorbed") {
        std::vector<std::vector<int32_t>> clauses = {
                {1, 2},
                {-1, 2, 3},
                {2, -3},
        };
        SECTION("With conversion") {
            WatchedLiterals formula = WatchedLiterals::from_vector(clauses);
            CHECK(with::is_clause_absorbed(formula, {1, 2, 3}));

            formula.backtrack_to(0);
            CHECK(with::is_clause_absorbed(formula, {-1, 2, -3}));
        }
        SECTION("Without conversion") {
            SylvanZddCnf cnf = SylvanZddCnf::from_vector(clauses);
            CHECK(without::is_clause_absorbed(cnf, {1, 2, 3}));
            CHECK(without::is_clause_absorbed(cnf, {-1, 2, -3}));
        }
    }

    SECTION("Tautology is always absorbed") {
        std::vector<std::vector<int32_t>> clauses = {
                {1, -2},
                {2, 3},
        };
        SECTION("With conversion") {
            WatchedLiterals formula = WatchedLiterals::from_vector(clauses);
            CHECK(with::is_clause_absorbed(formula, {1, -1}));

            formula.backtrack_to(0);
            CHECK(with::is_clause_absorbed(formula, {2, -2}));

            formula.backtrack_to(0);
            CHECK(with::is_clause_absorbed(formula, {1, -1, -2, 3}));
        }
        SECTION("Without conversion") {
            SylvanZddCnf cnf = SylvanZddCnf::from_vector(clauses);
            CHECK(without::is_clause_absorbed(cnf, {1, -1}));
            CHECK(without::is_clause_absorbed(cnf, {2, -2}));
            CHECK(without::is_clause_absorbed(cnf, {1, -1, -2, 3}));
        }
    }

    SECTION("Empty clause always absorbed") {
        std::vector<std::vector<int32_t>> clauses = {
                {1, -2},
                {-1, 2}
        };
        SECTION("With conversion") {
            WatchedLiterals formula = WatchedLiterals::from_vector(clauses);
            CHECK(with::is_clause_absorbed(formula, {}));
        }
        SECTION("Without conversion") {
            SylvanZddCnf cnf = SylvanZddCnf::from_vector(clauses);
            CHECK(without::is_clause_absorbed(cnf, {}));
        }
    }
}

TEST_CASE("remove_absorbed_clauses algorithm", "[absorbed clause detection]") {
    namespace without = dp::absorbed_clause_detection::without_conversion;
    namespace with = dp::absorbed_clause_detection::with_conversion;

    SECTION("No absorbed clauses") {
        std::vector<std::vector<int32_t>> clauses = {
                {1, -2},
                {-1, 2, 3},
                {-3, 4},
        };
        auto result = with::remove_absorbed_clauses_impl(clauses);
        CHECK(result.size() == clauses.size());
    }

    SECTION("Single absorbed clause") {
        std::vector<std::vector<int32_t>> clauses = {
                {1, -2},
                {-1, 2, 3}, // absorbed
                {-1, 2},
        };
        auto result = with::remove_absorbed_clauses_impl(clauses);
        CHECK(result.size() == 2);
        CHECK(std::find(result.begin(), result.end(), std::vector<int32_t>{-1, 2, 3}) == result.end());
    }

    SECTION("Multiple absorbed clauses") {
        std::vector<std::vector<int32_t>> clauses = {
                {1, -2},
                {1, -2, 3},     // absorbed
                {1, -2, -4},    // absorbed
                {-1, 2, -3, 4},
        };
        auto result = with::remove_absorbed_clauses_impl(clauses);
        CHECK(result.size() == 2);
    }

    SECTION("All clauses absorbed except one") {
        std::vector<std::vector<int32_t>> clauses = {
                {1},
                {1, -2},        // absorbed
                {1, 2, -3},     // absorbed
                {1, -2, 3, -4}, // absorbed
        };
        auto result = with::remove_absorbed_clauses_impl(clauses);
        REQUIRE(result.size() == 1);
        CHECK(result[0] == std::vector<int32_t>{1});
    }

    SECTION("Complex case with no absorbed clauses") {
        std::vector<std::vector<int32_t>> clauses = {
                {1, 2, 3},
                {-1, -2},
                {-2, -3},
                {3, 4},
                {-1, 4, -5},
        };
        auto result = with::remove_absorbed_clauses_impl(clauses);
        CHECK(result.size() == clauses.size());
    }
}

TEST_CASE("unify_with_non_absorbed algorithm", "[absorbed clause detection]") {
    namespace without = dp::absorbed_clause_detection::without_conversion;
    namespace with = dp::absorbed_clause_detection::with_conversion;
    using namespace dp;

    std::vector<std::vector<int32_t>> clauses_stable = {
            {1, -2},
            {-3, 4},
    };
    auto cnf_stable = SylvanZddCnf::from_vector(clauses_stable);

    SECTION("No absorbed clauses") {
        std::vector<std::vector<int32_t>> clauses_checked = {
                {-1, 2, 3},
        };
        auto cnf_checked = SylvanZddCnf::from_vector(clauses_checked);
        auto result = with::unify_with_non_absorbed(cnf_stable, cnf_checked);
        auto expected = SylvanZddCnf::from_vector({
            {1, -2},
            {-3, 4},
            {-1, 2, 3},
        });
        CHECK(result == expected);
    }

    SECTION("Single absorbed clause") {
        std::vector<std::vector<int32_t>> clauses_checked = {
                {-1, 2, 3},
                {1, -2, 3}, // absorbed
        };
        auto cnf_checked = SylvanZddCnf::from_vector(clauses_checked);
        auto result = with::unify_with_non_absorbed(cnf_stable, cnf_checked);
        auto expected = SylvanZddCnf::from_vector({
            {1, -2},
            {-3, 4},
            {-1, 2, 3},
        });
        CHECK(result == expected);
    }

    SECTION("Multiple absorbed clauses") {
        std::vector<std::vector<int32_t>> clauses_checked = {
                {1, -2, 3},     // absorbed
                {-1, 2},
                {-1, 2, -4},    // absorbed
        };
        auto cnf_checked = SylvanZddCnf::from_vector(clauses_checked);
        auto result = with::unify_with_non_absorbed(cnf_stable, cnf_checked);
        auto expected = SylvanZddCnf::from_vector({
            {1, -2},
            {-3, 4},
            {-1, 2},
        });
        CHECK(result == expected);
    }

    SECTION("All clauses absorbed") {
        std::vector<std::vector<int32_t>> clauses_checked = {
                {1, -2},        // absorbed
                {1, 2, -3, 4},  // absorbed
                {1, -2, 3, -4}, // absorbed
        };
        auto cnf_checked = SylvanZddCnf::from_vector(clauses_checked);
        auto result = with::unify_with_non_absorbed(cnf_stable, cnf_checked);
        auto expected = SylvanZddCnf::from_vector({
            {1, -2},
            {-3, 4},
        });
        CHECK(result == expected);
    }

}
