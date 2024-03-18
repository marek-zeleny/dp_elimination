#include <catch2/catch_test_macros.hpp>
#include "data_structures/sylvan_zdd_cnf.hpp"

#include <sylvan_obj.hpp>
#include <filesystem>
#include <algorithm>

TEST_CASE("SylvanZddCnf operations", "[SylvanZddCnf]") {
    using namespace dp;

    SylvanZddCnf::Clause clause1 = {1, -2, 3};
    SylvanZddCnf::Clause clause2 = {-1, 2, -3};
    std::vector<SylvanZddCnf::Clause> clauses = {clause1, clause2};

    SECTION("Construction from vector") {
        SECTION("Simple case") {
            SylvanZddCnf cnf = SylvanZddCnf::from_vector({
                {1, -2, 3},
                {-1, 2, -3},
            });
            CHECK_FALSE(cnf.is_empty());
            CHECK(cnf.clauses_count() == 2);
        }

        SECTION("Order of variables doesn't matter") {
            SylvanZddCnf cnf1 = SylvanZddCnf::from_vector({{1, -2, 3}});
            SylvanZddCnf cnf2 = SylvanZddCnf::from_vector({{-2, 3, 1}});
            SylvanZddCnf cnf3 = SylvanZddCnf::from_vector({{3, -2, 1}});
            CHECK_FALSE(cnf1.is_empty());
            CHECK(cnf1.clauses_count() == 1);
            CHECK(cnf1 == cnf2);
            CHECK(cnf1 == cnf3);
        }

        SECTION("Order of clauses doesn't matter") {
            SylvanZddCnf cnf1 = SylvanZddCnf::from_vector({{1}, {-2}, {3}});
            SylvanZddCnf cnf2 = SylvanZddCnf::from_vector({{-2}, {3}, {1}});
            SylvanZddCnf cnf3 = SylvanZddCnf::from_vector({{3}, {-2}, {1}});
            CHECK_FALSE(cnf1.is_empty());
            CHECK(cnf1.clauses_count() == 3);
            CHECK(cnf1 == cnf2);
            CHECK(cnf1 == cnf3);
        }
    }

    SECTION("Conversion to vector") {
        std::vector<SylvanZddCnf::Clause> clauses = {
                {1, -2, 3},
                {-1, 2, -3},
        };
        SylvanZddCnf cnf = SylvanZddCnf::from_vector(clauses);
        auto vector = cnf.to_vector();
        std::sort(vector.begin(), vector.end());
        std::sort(clauses.begin(), clauses.end());
        CHECK(vector == clauses);
    }

    SECTION("Equality operator") {
        SylvanZddCnf cnf1 = SylvanZddCnf::from_vector({
            {1, -2, 3},
            {-1, 2, -3},
        });
        SylvanZddCnf cnf2 = SylvanZddCnf::from_vector({
            {-1, 2, -3},
            {1, -2, 3},
        });
        SylvanZddCnf cnf3 = cnf1;
        SylvanZddCnf empty_cnf = SylvanZddCnf::from_vector({});
        SylvanZddCnf different_cnf = SylvanZddCnf::from_vector({
            {-1, 2, -3},
        });

        SECTION("Reflectiveness") {
            CHECK(cnf1 == cnf1);
            CHECK(empty_cnf == empty_cnf);
            CHECK(different_cnf == different_cnf);
        }

        SECTION("Formulas with identical clauses are equal") {
            CHECK(cnf1 == cnf2);
            CHECK(cnf1 == cnf3);
            CHECK(cnf2 == cnf3);
        }

        SECTION("Formulas with different clauses are not equal") {
            CHECK(cnf1 != different_cnf);
            CHECK(cnf1 != empty_cnf);
            CHECK(different_cnf != empty_cnf);
        }
    }

    SECTION("Property queries") {
        SylvanZddCnf cnf = SylvanZddCnf::from_vector({
            {1, -2, 3},
            {-1, 2, -3},
        });
        SylvanZddCnf empty_cnf1 = SylvanZddCnf::from_vector({});
        SylvanZddCnf empty_cnf2;
        SylvanZddCnf cnf_with_empty_clause1 = SylvanZddCnf::from_vector({{}});
        SylvanZddCnf cnf_with_empty_clause2 = SylvanZddCnf::from_vector({
            {1, -2, 3},
            {-1, 2, -3},
            {},
        });

        SECTION("Empty formulas") {
            CHECK(empty_cnf1 == empty_cnf2);

            CHECK(empty_cnf1.is_empty());
            CHECK(empty_cnf2.is_empty());

            CHECK_FALSE(cnf.is_empty());
            CHECK_FALSE(cnf_with_empty_clause1.is_empty());
            CHECK_FALSE(cnf_with_empty_clause2.is_empty());
        }

        SECTION("Empty clauses") {
            CHECK(cnf_with_empty_clause1.contains_empty());
            CHECK(cnf_with_empty_clause2.contains_empty());

            CHECK_FALSE(cnf.contains_empty());
            CHECK_FALSE(empty_cnf1.contains_empty());
            CHECK_FALSE(empty_cnf2.contains_empty());
        }

        SECTION("Smallest and largest variables") {
            CHECK(cnf.get_smallest_variable() == 1);
            CHECK(cnf.get_largest_variable() == 3);
            CHECK(cnf_with_empty_clause2.get_smallest_variable() == 1);
            CHECK(cnf_with_empty_clause2.get_largest_variable() == 3);
        }

        SECTION("Empty or zero clauses have no extreme variables") {
            CHECK(empty_cnf1.get_smallest_variable() == 0);
            CHECK(empty_cnf1.get_largest_variable() == 0);
            CHECK(cnf_with_empty_clause1.get_smallest_variable() == 0);
            CHECK(cnf_with_empty_clause1.get_largest_variable() == 0);
        }
    }

    SECTION("Unit and clear literal detection") {
        SylvanZddCnf empty_cnf;
        SylvanZddCnf cnf_with_empty_clause1 = SylvanZddCnf::from_vector({{}});
        SylvanZddCnf cnf_with_empty_clause2 = SylvanZddCnf::from_vector({
            {},
            {1},
            {-1, 2},
        });
        SylvanZddCnf cnf_with_multiple = SylvanZddCnf::from_vector({
            {-1, 2, 4},
            {1, 2},
            {2, -3, 4},
            {1},
            {3},
        });
        SylvanZddCnf cnf_without_any = SylvanZddCnf::from_vector({
            {-1, 2, 3},
            {1, -2},
            {1, 2, -3},
        });
        SylvanZddCnf::Literal unit;
        SylvanZddCnf::Literal clear;

        SECTION("Unit literals") {
            unit = empty_cnf.get_unit_literal();
            CHECK(unit == 0);
            unit = cnf_with_empty_clause1.get_unit_literal();
            CHECK(unit == 0);
            unit = cnf_with_empty_clause2.get_unit_literal();
            CHECK(unit == 1);
            unit = cnf_with_multiple.get_unit_literal();
            CHECK((unit == 1 || unit == 3));
            unit = cnf_without_any.get_unit_literal();
            CHECK(unit == 0);
        }

        SECTION("Clear literals") {
            clear = empty_cnf.get_clear_literal();
            CHECK(clear == 0);
            clear = cnf_with_empty_clause1.get_clear_literal();
            CHECK(clear == 0);
            clear = cnf_with_empty_clause2.get_clear_literal();
            CHECK(clear == 2);
            clear = cnf_with_multiple.get_clear_literal();
            CHECK((clear == 2 || clear == 4));
            clear = cnf_without_any.get_clear_literal();
            CHECK(clear == 0);
        }
    }

    SECTION("Formula statistics collection") {
        SECTION("Simple case") {
            SylvanZddCnf cnf = SylvanZddCnf::from_vector({
                {},
                {1},
                {-1, 2},
            });
            SylvanZddCnf::FormulaStats stats = cnf.get_formula_statistics();
            CHECK(stats.index_shift == 1);
            REQUIRE(stats.vars.size() == 2);
            CHECK(stats.vars[0].positive_clause_count == 1);
            CHECK(stats.vars[0].negative_clause_count == 1);
            CHECK(stats.vars[1].positive_clause_count == 1);
            CHECK(stats.vars[1].negative_clause_count == 0);
        }

        SECTION("Complex case") {
            SylvanZddCnf cnf = SylvanZddCnf::from_vector({
                {-11, 12, 14},
                {11,  12},
                {12,  -14},
                {11},
                {14},
            });
            SylvanZddCnf::FormulaStats stats = cnf.get_formula_statistics();
            CHECK(stats.index_shift == 11);
            REQUIRE(stats.vars.size() == 4);
            CHECK(stats.vars[0].positive_clause_count == 2);
            CHECK(stats.vars[0].negative_clause_count == 1);
            CHECK(stats.vars[1].positive_clause_count == 3);
            CHECK(stats.vars[1].negative_clause_count == 0);
            CHECK(stats.vars[2].positive_clause_count == 0);
            CHECK(stats.vars[2].negative_clause_count == 0);
            CHECK(stats.vars[3].positive_clause_count == 2);
            CHECK(stats.vars[3].negative_clause_count == 1);
        }
    }

    SECTION("Set operations") {
        SylvanZddCnf cnf = SylvanZddCnf::from_vector({
            {1, -2, 3},
            {-1, 2, -3},
        });
        SylvanZddCnf expected;

        SECTION("Subset0") {
            auto result1 = cnf.subset0(-1);
            expected = SylvanZddCnf::from_vector({
                {1, -2, 3},
            });
            CHECK(result1 == expected);

            auto result2 = cnf.subset0(1);
            expected = SylvanZddCnf::from_vector({
                {-1, 2, -3},
            });
            CHECK(result2 == expected);
        }

        SECTION("Subset1") {
            auto result1 = cnf.subset1(-1);
            expected = SylvanZddCnf::from_vector({
                {2, -3},
            });
            CHECK(result1 == expected);

            auto result2 = cnf.subset1(1);
            expected = SylvanZddCnf::from_vector({
                {-2, 3},
            });
            CHECK(result2 == expected);
        }

        SylvanZddCnf operand1 = SylvanZddCnf::from_vector({
            {1, -2, 3},
        });
        SylvanZddCnf operand2 = SylvanZddCnf::from_vector({
            {-2, 3},
        });

        SECTION("Union") {
            auto result1 = cnf.unify(operand1);
            CHECK(result1 == cnf);

            auto result2 = cnf.unify(operand2);
            expected = SylvanZddCnf::from_vector({
                {1, -2, 3},
                {-1, 2, -3},
                {-2, 3},
            });
            CHECK(result2 == expected);
        }

        SECTION("Intersection") {
            auto result1 = cnf.intersect(operand1);
            CHECK(result1 == operand1);

            auto result2 = cnf.intersect(operand2);
            expected = SylvanZddCnf();
            CHECK(result2 == expected);
        }

        SECTION("Subtraction") {
            auto result1 = cnf.subtract(operand1);
            expected = SylvanZddCnf::from_vector({
                {-1, 2, -3},
            });
            CHECK(result1 == expected);

            auto result2 = cnf.subtract(operand2);
            CHECK(result2 == cnf);
        }

        SECTION("Multiplication") {
            auto result = cnf.multiply(operand2);
            expected = SylvanZddCnf::from_vector({
                {1, -2, 3},
                {-1, 2, -2, 3, -3},
            });
            CHECK(result == expected);
        }
    }

    SECTION("Removing tautologies") {
        SylvanZddCnf cnf = SylvanZddCnf::from_vector({
            {},
            {1, -1},
            {-1, 2, 3},
            {-2, 3, -3},
        });
        auto no_tautologies = cnf.remove_tautologies();
        auto expected = SylvanZddCnf::from_vector({
            {},
            {-1, 2, 3},
        });
        CHECK(no_tautologies == expected);
    }

    SECTION("Removing subsumed clauses") {
        SylvanZddCnf cnf = SylvanZddCnf::from_vector({
            {1, -2, 3},
            {-1},
            {-2, 3},
            {-1, -2, -3},
        });
        auto no_subsumed = cnf.remove_subsumed_clauses();
        auto expected = SylvanZddCnf::from_vector({
            {-1},
            {-2, 3},
        });
        CHECK(no_subsumed == expected);
    }

    SECTION("File operations") {
        SylvanZddCnf cnf = SylvanZddCnf::from_vector({
            {1, -2, 3},
            {-1, 2, -3},
        });

        SECTION("Draw as dot file") {
            std::string file_name = ".test_zdd.dot";
            CHECK(cnf.draw_to_file(file_name));
            REQUIRE(std::filesystem::remove(file_name.c_str()));
        }

        SECTION("Write as DIMACS file") {
            std::string file_name = ".test_dimacs.cnf";
            CHECK(cnf.write_dimacs_to_file(file_name));
            REQUIRE(std::filesystem::remove(file_name.c_str()));
        }
    }
}
