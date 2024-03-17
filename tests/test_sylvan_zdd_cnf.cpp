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
        SylvanZddCnf cnf = SylvanZddCnf::from_vector(clauses);
        REQUIRE_FALSE(cnf.is_empty());
        REQUIRE(cnf.clauses_count() == 2);

        SylvanZddCnf cnf1 = SylvanZddCnf::from_vector({{1, -2, 3}});
        SylvanZddCnf cnf2 = SylvanZddCnf::from_vector({{-2, 3, 1}});
        SylvanZddCnf cnf3 = SylvanZddCnf::from_vector({{3, -2, 1}});
        REQUIRE_FALSE(cnf1.is_empty());
        REQUIRE(cnf1.clauses_count() == 1);
        REQUIRE(cnf1 == cnf2);
        REQUIRE(cnf1 == cnf3);
    }

    SECTION("Conversion to vector") {
        SylvanZddCnf cnf = SylvanZddCnf::from_vector(clauses);
        auto vector = cnf.to_vector();
        std::sort(vector.begin(), vector.end());
        auto expected = clauses;
        std::sort(expected.begin(), expected.end());
        REQUIRE(vector == expected);
    }

    SECTION("Equality operator") {
        SylvanZddCnf cnf1 = SylvanZddCnf::from_vector(clauses);
        SylvanZddCnf cnf2 = SylvanZddCnf::from_vector({clause2, clause1});
        SylvanZddCnf cnf3 = cnf1;
        SylvanZddCnf empty_cnf = SylvanZddCnf::from_vector({});
        SylvanZddCnf different_cnf = SylvanZddCnf::from_vector({clause1});

        REQUIRE(cnf1 == cnf1);
        REQUIRE(empty_cnf == empty_cnf);
        REQUIRE(different_cnf == different_cnf);

        REQUIRE(cnf1 == cnf2);
        REQUIRE(cnf1 == cnf3);
        REQUIRE(cnf2 == cnf3);
        REQUIRE(cnf1 != different_cnf);
        REQUIRE(cnf1 != empty_cnf);
        REQUIRE(different_cnf != empty_cnf);
    }

    SECTION("Property queries") {
        SylvanZddCnf cnf = SylvanZddCnf::from_vector(clauses);
        SylvanZddCnf empty_cnf1 = SylvanZddCnf::from_vector({});
        SylvanZddCnf empty_cnf2;
        SylvanZddCnf cnf_with_empty_clause1 = SylvanZddCnf::from_vector({{}});
        SylvanZddCnf cnf_with_empty_clause2 = SylvanZddCnf::from_vector({clause1, clause2, {}});

        REQUIRE_FALSE(cnf.is_empty());
        REQUIRE_FALSE(cnf.contains_empty());

        REQUIRE(empty_cnf1.is_empty());
        REQUIRE(empty_cnf2.is_empty());
        REQUIRE_FALSE(empty_cnf1.contains_empty());
        REQUIRE_FALSE(empty_cnf2.contains_empty());
        REQUIRE(empty_cnf1 == empty_cnf2);

        REQUIRE_FALSE(cnf_with_empty_clause1.is_empty());
        REQUIRE_FALSE(cnf_with_empty_clause2.is_empty());
        REQUIRE(cnf_with_empty_clause1.contains_empty());
        REQUIRE(cnf_with_empty_clause2.contains_empty());

        REQUIRE(cnf.get_smallest_variable() == 1);
        REQUIRE(cnf.get_largest_variable() == 3);
        REQUIRE(cnf_with_empty_clause2.get_smallest_variable() == 1);
        REQUIRE(cnf_with_empty_clause2.get_largest_variable() == 3);

        REQUIRE(empty_cnf1.get_smallest_variable() == 0);
        REQUIRE(empty_cnf1.get_largest_variable() == 0);
        REQUIRE(cnf_with_empty_clause1.get_smallest_variable() == 0);
        REQUIRE(cnf_with_empty_clause1.get_largest_variable() == 0);
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

        unit = empty_cnf.get_unit_literal();
        REQUIRE(unit == 0);
        unit = cnf_with_empty_clause1.get_unit_literal();
        REQUIRE(unit == 0);
        unit = cnf_with_empty_clause2.get_unit_literal();
        REQUIRE(unit == 1);
        unit = cnf_with_multiple.get_unit_literal();
        REQUIRE((unit == 1 || unit == 3));
        unit = cnf_without_any.get_unit_literal();
        REQUIRE(unit == 0);

        clear = empty_cnf.get_clear_literal();
        REQUIRE(clear == 0);
        clear = cnf_with_empty_clause1.get_clear_literal();
        REQUIRE(clear == 0);
        clear = cnf_with_empty_clause2.get_clear_literal();
        REQUIRE(clear == 2);
        clear = cnf_with_multiple.get_clear_literal();
        REQUIRE((clear == 2 || clear == 4));
        clear = cnf_without_any.get_clear_literal();
        REQUIRE(clear == 0);
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
        SylvanZddCnf cnf = SylvanZddCnf::from_vector(clauses);
        SylvanZddCnf expected;

        auto subset0 = cnf.subset0(-1);
        expected = SylvanZddCnf::from_vector({clause1});
        REQUIRE(subset0 == expected);

        subset0 = cnf.subset0(1);
        expected = SylvanZddCnf::from_vector({clause2});
        REQUIRE(subset0 == expected);

        auto subset1 = cnf.subset1(-1);
        expected = SylvanZddCnf::from_vector({{2, -3}});
        REQUIRE(subset1 == expected);

        subset1 = cnf.subset1(1);
        expected = SylvanZddCnf::from_vector({{-2, 3}});
        REQUIRE(subset1 == expected);

        auto unified = cnf.unify(subset1);
        expected = SylvanZddCnf::from_vector({clause1, clause2, {-2, 3}});
        REQUIRE(unified == expected);

        auto intersected = cnf.intersect(subset0);
        expected = SylvanZddCnf::from_vector({clause2});
        REQUIRE(intersected == expected);

        auto subtracted = cnf.subtract(subset0);
        expected = SylvanZddCnf::from_vector({clause1});
        REQUIRE(subtracted == expected);

        auto multiplied = cnf.multiply(subset1);
        expected = SylvanZddCnf::from_vector({clause1, {-1, 2, -2, 3, -3}});
        REQUIRE(multiplied == expected);
    }

    SECTION("Removing tautologies") {
        SylvanZddCnf cnf = SylvanZddCnf::from_vector({{}, {1, -1}, {-1, 2, 3}, {-2, 3, -3}});

        auto no_tautologies = cnf.remove_tautologies();
        auto expected = SylvanZddCnf::from_vector({{}, {-1, 2, 3}});
        REQUIRE(no_tautologies == expected);
    }

    SECTION("Removing subsumed clauses") {
        SylvanZddCnf cnf = SylvanZddCnf::from_vector({{1, -2, 3}, {-1}, {-2, 3}, {-1, -2, -3}});

        auto no_subsumed = cnf.remove_subsumed_clauses();
        auto expected = SylvanZddCnf::from_vector({{-1}, {-2, 3}});
        REQUIRE(no_subsumed == expected);
    }

    SECTION("File operations") {
        SylvanZddCnf cnf = SylvanZddCnf::from_vector(clauses);

        std::string dot_file_name = "test_zdd.dot";
        REQUIRE(cnf.draw_to_file(dot_file_name));
        std::filesystem::remove(dot_file_name.c_str());

        std::string dimacs_file_name = "test_dimacs.cnf";
        REQUIRE(cnf.write_dimacs_to_file(dimacs_file_name));
        std::filesystem::remove(dimacs_file_name.c_str());
    }
}
