#include <catch2/catch_test_macros.hpp>
#include "data_structures/sylvan_zdd_cnf.hpp"

#include <sylvan_obj.hpp>
#include <filesystem>

TEST_CASE("SylvanZddCnf operations", "[SylvanZddCnf]") {
    using namespace dp;

    SylvanZddCnf::Clause clause1 = {1, -2, 3};
    SylvanZddCnf::Clause clause2 = {-1, 2, -3};
    std::vector<SylvanZddCnf::Clause> clauses = {clause1, clause2};

    SECTION("Construction from vector") {
        SylvanZddCnf zddCnf = SylvanZddCnf::from_vector(clauses);
        REQUIRE_FALSE(zddCnf.is_empty());
        REQUIRE(zddCnf.clauses_count() == 2);
    }

    SECTION("Set operations") {
        SylvanZddCnf zddCnf = SylvanZddCnf::from_vector(clauses);
        SylvanZddCnf expected;

        auto subset0 = zddCnf.subset0(1);
        expected = SylvanZddCnf::from_vector({clause2});
        REQUIRE(subset0 == expected);

        auto subset1 = zddCnf.subset1(1);
        expected = SylvanZddCnf::from_vector({{-2, 3}});
        REQUIRE(subset1 == expected);

        auto unified = zddCnf.unify(subset1);
        expected = SylvanZddCnf::from_vector({clause1, clause2, {-2, 3}});
        REQUIRE(unified == expected);

        auto intersected = zddCnf.intersect(subset0);
        expected = SylvanZddCnf::from_vector({clause2});
        REQUIRE(intersected == expected);

        auto subtracted = zddCnf.subtract(subset0);
        expected = SylvanZddCnf::from_vector({clause1});
        REQUIRE(subtracted == expected);

        auto multiplied = zddCnf.multiply(subset1);
        expected = SylvanZddCnf::from_vector({clause1, {-1, 2, -2, 3, -3}});
        REQUIRE(multiplied == expected);
    }

    SECTION("Removing tautologies") {
        SylvanZddCnf zddCnf = SylvanZddCnf::from_vector({{}, {1, -1}, {-1, 2, 3}, {-2, 3, -3}});

        auto no_tautologies = zddCnf.remove_tautologies();
        auto expected = SylvanZddCnf::from_vector({{}, {-1, 2, 3}});
        REQUIRE(no_tautologies == expected);
    }

    SECTION("Removing subsumed clauses") {
        SylvanZddCnf zddCnf = SylvanZddCnf::from_vector({{1, -2, 3}, {-1}, {-2, 3}, {-1, -2, -3}});

        auto no_subsumed = zddCnf.remove_subsumed_clauses();
        auto expected = SylvanZddCnf::from_vector({{-1}, {-2, 3}});
        REQUIRE(no_subsumed == expected);
    }

    SECTION("Variable retrieval") {
        SylvanZddCnf zddCnf = SylvanZddCnf::from_vector(clauses);
        REQUIRE(zddCnf.get_smallest_variable() == 1);
        REQUIRE(zddCnf.get_largest_variable() == 3);
    }

    SECTION("File operations") {
        SylvanZddCnf zddCnf = SylvanZddCnf::from_vector(clauses);

        std::string dot_file_name = "test_zdd.dot";
        REQUIRE(zddCnf.draw_to_file(dot_file_name));
        std::filesystem::remove(dot_file_name.c_str());

        std::string dimacs_file_name = "test_dimacs.cnf";
        REQUIRE(zddCnf.write_dimacs_to_file(dimacs_file_name));
        std::filesystem::remove(dimacs_file_name.c_str());
    }
}
