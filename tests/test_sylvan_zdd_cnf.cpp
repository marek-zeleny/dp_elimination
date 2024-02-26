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

    SECTION("Set operations") {
        SylvanZddCnf cnf = SylvanZddCnf::from_vector(clauses);
        SylvanZddCnf expected;

        auto subset0 = cnf.subset0(1);
        expected = SylvanZddCnf::from_vector({clause2});
        REQUIRE(subset0 == expected);

        auto subset1 = cnf.subset1(1);
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
