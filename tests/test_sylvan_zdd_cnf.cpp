#include <catch2/catch_test_macros.hpp>
#include "data_structures/sylvan_zdd_cnf.hpp"

#include <filesystem>
#include <algorithm>

#include "data_structures/vector_cnf.hpp"

TEST_CASE("SylvanZddCnf operations", "[SylvanZddCnf]") {
    using namespace dp;
    using Clauses = std::vector<std::vector<int32_t>>;

    SECTION("Construction from vector") {
        SECTION("Simple case") {
            SylvanZddCnf cnf = SylvanZddCnf::from_vector({
                {1, -2, 3},
                {-1, 2, -3},
            });
            CHECK_FALSE(cnf.is_empty());
            CHECK(cnf.count_clauses() == 2);
        }

        SECTION("Order of variables doesn't matter") {
            SylvanZddCnf cnf1 = SylvanZddCnf::from_vector({{1, -2, 3}});
            SylvanZddCnf cnf2 = SylvanZddCnf::from_vector({{-2, 3, 1}});
            SylvanZddCnf cnf3 = SylvanZddCnf::from_vector({{3, -2, 1}});
            CHECK_FALSE(cnf1.is_empty());
            CHECK(cnf1.count_clauses() == 1);
            CHECK(cnf1 == cnf2);
            CHECK(cnf1 == cnf3);
        }

        SECTION("Order of clauses doesn't matter") {
            SylvanZddCnf cnf1 = SylvanZddCnf::from_vector({{1}, {-2}, {3}});
            SylvanZddCnf cnf2 = SylvanZddCnf::from_vector({{-2}, {3}, {1}});
            SylvanZddCnf cnf3 = SylvanZddCnf::from_vector({{3}, {-2}, {1}});
            CHECK_FALSE(cnf1.is_empty());
            CHECK(cnf1.count_clauses() == 3);
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
        SylvanZddCnf cnf1 = SylvanZddCnf::from_vector({
            {},
            {1},
            {-1, 2},
        });

        SylvanZddCnf cnf2 = SylvanZddCnf::from_vector({
            {-11, 12, 14},
            {11,  12},
            {12,  -14},
            {11},
            {14},
        });

        SECTION("Find literals") {
            SECTION("Simple case") {
                SylvanZddCnf::FormulaStats stats = cnf1.find_all_literals();
                CHECK(stats.index_shift == 1);
                REQUIRE(stats.vars.size() == 2);
                CHECK(stats.vars[0].positive_clause_count > 0);
                CHECK(stats.vars[0].negative_clause_count > 0);
                CHECK(stats.vars[1].positive_clause_count > 0);
                CHECK(stats.vars[1].negative_clause_count == 0);
            }

            SECTION("Complex case") {
                SylvanZddCnf::FormulaStats stats = cnf2.find_all_literals();
                CHECK(stats.index_shift == 11);
                REQUIRE(stats.vars.size() == 4);
                CHECK(stats.vars[0].positive_clause_count > 0);
                CHECK(stats.vars[0].negative_clause_count > 0);
                CHECK(stats.vars[1].positive_clause_count > 0);
                CHECK(stats.vars[1].negative_clause_count == 0);
                CHECK(stats.vars[2].positive_clause_count == 0);
                CHECK(stats.vars[2].negative_clause_count == 0);
                CHECK(stats.vars[3].positive_clause_count > 0);
                CHECK(stats.vars[3].negative_clause_count > 0);
            }
        }

        SECTION("Count literals") {
            SECTION("Simple case") {
                SylvanZddCnf::FormulaStats stats = cnf1.count_all_literals();
                CHECK(stats.index_shift == 1);
                REQUIRE(stats.vars.size() == 2);
                CHECK(stats.vars[0].positive_clause_count == 1);
                CHECK(stats.vars[0].negative_clause_count == 1);
                CHECK(stats.vars[1].positive_clause_count == 1);
                CHECK(stats.vars[1].negative_clause_count == 0);
            }

            SECTION("Complex case") {
                SylvanZddCnf::FormulaStats stats = cnf2.count_all_literals();
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
    }

    SECTION("Subset operations (not implemented by Sylvan)") {
        Clauses input = {
                {1,  -2, 3},
                {-1, 2,  -3},
        };
        Clauses large_input = {
                {1},
                {1,  5},
                {1,  5, -7},
                {-1},
                {-1, 7},
                {2},
                {2,  3, -5},
                {-2, 5},
                {-3, 5},
                {4},
                {4,  -5},
                {-4, 5},
                {-5},
                {-5, 7},
                {6},
                {6,  -7},
                {-6},
                {-6, 7},
                {7,  11},
                {-7, 12},
                {8,  11},
                {-8},
                {-8, 10},
                {-9},
                {-9, 10},
                {10, 12},
                {11},
        };
        auto cnf = SylvanZddCnf::from_vector(input);
        auto large_cnf = SylvanZddCnf::from_vector(large_input);
        auto vec = VectorCnf::from_vector(input);
        auto large_vec = VectorCnf::from_vector(large_input);
        SylvanZddCnf expected;

        CHECK(large_cnf.verify_variable_ordering());

        SECTION("Subset0") {
            auto result1 = cnf.subset0(-1);
            expected = SylvanZddCnf::from_vector({
                {1, -2, 3},
            });
            CHECK(result1 == expected);
            auto tmp = vec;
            CHECK(result1 == SylvanZddCnf::from_vector(tmp.subset0(-1).to_vector()));

            auto result2 = cnf.subset0(-3);
            CHECK(result2 == expected);
            tmp = vec;
            CHECK(result2 == SylvanZddCnf::from_vector(tmp.subset0(-3).to_vector()));

            auto result3 = cnf.subset0(1);
            expected = SylvanZddCnf::from_vector({
                {-1, 2, -3},
            });
            CHECK(result3 == expected);
            tmp = vec;
            CHECK(result3 == SylvanZddCnf::from_vector(tmp.subset0(1).to_vector()));

            auto result4 = cnf.subset0(3);
            CHECK(result4 == expected);
            tmp = vec;
            CHECK(result4 == SylvanZddCnf::from_vector(tmp.subset0(3).to_vector()));

            auto result5 = large_cnf.subset0(10);
            expected = SylvanZddCnf::from_vector({
                {1},
                {1, 5},
                {1, 5, -7},
                {-1},
                {-1, 7},
                {2},
                {2, 3, -5},
                {-2, 5},
                {-3, 5},
                {4},
                {4, -5},
                {-4, 5},
                {-5},
                {-5, 7},
                {6},
                {6, -7},
                {-6},
                {-6, 7},
                {7, 11},
                {-7, 12},
                {8, 11},
                {-8},
                {-9},
                {11},
            });
            CHECK(result5.verify_variable_ordering());
            CHECK(result5 == expected);
            CHECK(result5 == SylvanZddCnf::from_vector(large_vec.subset0(10).to_vector()));
        }

        SECTION("Subset1") {
            auto result1 = cnf.subset1(-1);
            expected = SylvanZddCnf::from_vector({
                {2, -3},
            });
            CHECK(result1 == expected);
            auto tmp = vec;
            CHECK(result1 == SylvanZddCnf::from_vector(tmp.subset1(-1).to_vector()));

            auto result2 = cnf.subset1(-3);
            expected = SylvanZddCnf::from_vector({
                {-1, 2},
            });
            CHECK(result2 == expected);
            tmp = vec;
            CHECK(result2 == SylvanZddCnf::from_vector(tmp.subset1(-3).to_vector()));

            auto result3 = cnf.subset1(1);
            expected = SylvanZddCnf::from_vector({
                {-2, 3},
            });
            CHECK(result3 == expected);
            tmp = vec;
            CHECK(result3 == SylvanZddCnf::from_vector(tmp.subset1(1).to_vector()));

            auto result4 = cnf.subset1(3);
            expected = SylvanZddCnf::from_vector({
                {1, -2},
            });
            CHECK(result4 == expected);
            tmp = vec;
            CHECK(result4 == SylvanZddCnf::from_vector(tmp.subset1(3).to_vector()));

            auto result5 = large_cnf.subset1(10);
            expected = SylvanZddCnf::from_vector({
                {-8},
                {-9},
                {12},
            });
            CHECK(result5 == expected);
            CHECK(result5 == SylvanZddCnf::from_vector(large_vec.subset1(10).to_vector()));
        }

    }

    SECTION("Set operations (implemented by Sylvan)") {
        Clauses input = {
                {1,  -2, 3},
                {-1, 2,  -3},
        };
        Clauses operand1_input = {
                {1, -2, 3},
        };
        Clauses operand2_input = {
                {-2, 3},
        };

        auto cnf = SylvanZddCnf::from_vector(input);
        auto operand1_cnf = SylvanZddCnf::from_vector(operand1_input);
        auto operand2_cnf = SylvanZddCnf::from_vector(operand2_input);
        auto vec = VectorCnf::from_vector(input);
        auto operand1_vec = VectorCnf::from_vector(operand1_input);
        auto operand2_vec = VectorCnf::from_vector(operand2_input);
        SylvanZddCnf expected;

        SECTION("Union") {
            auto result1 = cnf.unify(operand1_cnf);
            CHECK(result1 == cnf);
            auto tmp = vec;
            CHECK(result1 == SylvanZddCnf::from_vector(tmp.unify(operand1_vec).to_vector()));

            auto result2 = cnf.unify(operand2_cnf);
            expected = SylvanZddCnf::from_vector({
                {1, -2, 3},
                {-1, 2, -3},
                {-2, 3},
            });
            CHECK(result2 == expected);
            CHECK(result2 == SylvanZddCnf::from_vector(vec.unify(operand2_vec).to_vector()));
        }

        SECTION("Intersection") {
            auto result1 = cnf.intersect(operand1_cnf);
            CHECK(result1 == operand1_cnf);
            auto tmp = vec;
            CHECK(result1 == SylvanZddCnf::from_vector(tmp.intersect(operand1_vec).to_vector()));

            auto result2 = cnf.intersect(operand2_cnf);
            expected = SylvanZddCnf();
            CHECK(result2 == expected);
            CHECK(result2 == SylvanZddCnf::from_vector(vec.intersect(operand2_vec).to_vector()));
        }

        SECTION("Subtraction") {
            auto result1 = cnf.subtract(operand1_cnf);
            expected = SylvanZddCnf::from_vector({
                {-1, 2, -3},
            });
            CHECK(result1 == expected);
            auto tmp = vec;
            CHECK(result1 == SylvanZddCnf::from_vector(tmp.subtract(operand1_vec).to_vector()));

            auto result2 = cnf.subtract(operand2_cnf);
            CHECK(result2 == cnf);
            CHECK(result2 == SylvanZddCnf::from_vector(vec.subtract(operand2_vec).to_vector()));
        }

        SECTION("Multiplication") {
            auto result = cnf.multiply(operand2_cnf);
            expected = SylvanZddCnf::from_vector({
                {1, -2, 3},
                {-1, 2, -2, 3, -3},
            });
            CHECK(result == expected);
            CHECK(result == SylvanZddCnf::from_vector(vec.multiply(operand2_vec).to_vector()));
        }
    }

    SECTION("Removing tautologies") {
        Clauses input = {
                {},
                {1,  -1},
                {-1, 2, 3},
                {-2, 3, -3},
        };
        auto cnf = SylvanZddCnf::from_vector(input);
        auto vec = VectorCnf::from_vector(input);

        auto no_tautologies = cnf.remove_tautologies();
        auto expected = SylvanZddCnf::from_vector({
            {},
            {-1, 2, 3},
        });
        CHECK(no_tautologies == expected);
        CHECK(no_tautologies == SylvanZddCnf::from_vector(vec.remove_tautologies().to_vector()));
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
            CHECK_NOTHROW(cnf.draw_to_file(file_name));
            REQUIRE(std::filesystem::remove(file_name.c_str()));
        }

        SECTION("Write as DIMACS file") {
            std::string file_name = ".test_dimacs.cnf";
            CHECK_NOTHROW(cnf.write_dimacs_to_file(file_name));
            REQUIRE(std::filesystem::remove(file_name.c_str()));
        }
    }
}
