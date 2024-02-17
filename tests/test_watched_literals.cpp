#include <catch2/catch_test_macros.hpp>
#include "data_structures/watched_literals.hpp"

TEST_CASE("WatchedLiterals operations", "[WatchedLiterals]") {
    using namespace dp;
    using Assignment = WatchedLiterals::Assignment;

    WatchedLiterals::Clause clause1 = {1, -2, 3};
    WatchedLiterals::Clause clause2 = {-1, 2, -3};
    WatchedLiterals::Clause clause3 = {-1, -2, 3};
    std::vector<WatchedLiterals::Clause> clauses = {clause1, clause2, clause3};

    SECTION("Initialization and basic assignments") {
        WatchedLiterals wl(clauses, 1, 3);

        REQUIRE(wl.get_assignment_level() == 0);
        REQUIRE(wl.assign_value(1));
        REQUIRE(wl.get_assignment(1) == Assignment::positive);
        REQUIRE(wl.assign_value(-2));
        REQUIRE(wl.get_assignment(-2) == Assignment::positive);
    }

    SECTION("Handling of unit clauses") {
        WatchedLiterals::Clause unitClause = {4};
        clauses.push_back(unitClause);
        WatchedLiterals wl(clauses, 1, 4);

        REQUIRE(wl.contains_empty() == false);
        REQUIRE(wl.get_assignment(4) == Assignment::positive);
    }

    SECTION("Conflict detection") {
        WatchedLiterals wl(clauses, 1, 3);

        REQUIRE(wl.assign_value(2));
        REQUIRE_FALSE(wl.assign_value(-3));
        REQUIRE(wl.contains_empty());
    }

    SECTION("Proper backtracking") {
        WatchedLiterals wl(clauses, 1, 3);
        REQUIRE(wl.assign_value(1));
        REQUIRE(wl.assign_value(-3));

        size_t levelBeforeBacktrack = wl.get_assignment_level();
        wl.backtrack(1);
        size_t levelAfterBacktrack = wl.get_assignment_level();

        REQUIRE(levelAfterBacktrack == levelBeforeBacktrack - 1);
        REQUIRE(wl.get_assignment(-3) == Assignment::unassigned);
    }

    SECTION("Backtracking to a specific level") {
        WatchedLiterals wl(clauses, 1, 3);
        REQUIRE(wl.assign_value(1));
        REQUIRE(wl.assign_value(-3));

        wl.backtrack_to(0);
        REQUIRE(wl.get_assignment_level() == 0);
        REQUIRE(wl.get_assignment(1) == Assignment::unassigned);
        REQUIRE(wl.get_assignment(-3) == Assignment::unassigned);
    }

    SECTION("Activation and deactivation of clauses") {
        std::unordered_set<size_t> deactivated = {2};
        WatchedLiterals wl(clauses, 1, 3, deactivated);

        // Initially, clause3 should not affect the assignments
        REQUIRE(wl.assign_value(2));
        REQUIRE(wl.assign_value(-3));
        REQUIRE(wl.get_assignment(1) == Assignment::positive);

        // Reactivate clause3 and check for a conflict
        wl.change_active_clauses({2}, {}); // Activate clause3
        REQUIRE(wl.get_assignment_level() == 0);
        REQUIRE(wl.assign_value(2));
        REQUIRE_FALSE(wl.assign_value(-3));
    }

    SECTION("Negation of assignments") {
        REQUIRE(WatchedLiterals::negate(Assignment::positive) == Assignment::negative);
        REQUIRE(WatchedLiterals::negate(Assignment::negative) == Assignment::positive);
        REQUIRE(WatchedLiterals::negate(Assignment::unassigned) == Assignment::unassigned);
    }

    SECTION("Static construction from clause vectors") {
        auto wl1 = WatchedLiterals::from_vector(clauses);
        REQUIRE_FALSE(wl1.contains_empty());

        std::unordered_set<size_t> deactivated = {2};
        auto wl2 = WatchedLiterals::from_vector(clauses, deactivated);
        REQUIRE_FALSE(wl2.contains_empty());
    }

    SECTION("Empty clause as input") {
        WatchedLiterals::Clause emptyClause = {};
        clauses.push_back(emptyClause);
        WatchedLiterals wl(clauses, 1, 5);

        REQUIRE(wl.contains_empty());
        // Assigning a value now shouldn't work
        REQUIRE_FALSE(wl.assign_value(1));
        REQUIRE_FALSE(wl.assign_value(-2));
    }
}
