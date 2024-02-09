#include <catch2/catch_test_macros.hpp>
#include "io/cnf_reader.hpp"

#include <sstream>
#include <vector>
#include <fstream>
#include <iostream>
#include <filesystem>

TEST_CASE("CnfReader functionality", "[CnfReader]") {
    std::string cnf_content;
    std::vector<dp::CnfReader::Clause> clauses;

    SECTION("Reads CNF from a valid stream") {
        cnf_content = "c This is a comment\n"
                      "p cnf 2 2\n"
                      "1 -2 0\n"
                      "2 0\n";
        std::istringstream stream(cnf_content);

        dp::CnfReader::AddClauseFunction func = [&](const dp::CnfReader::Clause &clause) {
            clauses.push_back(clause);
        };
        dp::CnfReader::read_from_stream(stream, func);

        REQUIRE(clauses.size() == 2);
        REQUIRE(clauses[0] == dp::CnfReader::Clause{1, -2});
        REQUIRE(clauses[1] == dp::CnfReader::Clause{2});
    }

    SECTION("Reading CNF from a file with missing problem definition throws exception") {
        cnf_content = "c This is a comment\n"
                      "1 -2 0\n"
                      "2 0\n";
        std::ofstream out("temp_cnf.cnf");
        out << cnf_content;
        out.close();

        REQUIRE_THROWS_AS(dp::CnfReader::read_from_file_to_vector("temp_cnf.cnf"), dp::CnfReader::failure);

        std::filesystem::remove("temp_cnf.cnf");
    }

    SECTION("Mismatch in declared and actual number of clauses generates warning") {
        // Redirect std::cerr to capture warnings
        std::stringstream captured_warnings;
        auto* original_cerr_buf = std::cerr.rdbuf(captured_warnings.rdbuf());

        cnf_content = "p cnf 2 3\n"
                      "1 -2 0\n"
                      "2 0\n";
        std::istringstream stream(cnf_content);

        dp::CnfReader::AddClauseFunction func = [&](const dp::CnfReader::Clause &clause) {
            clauses.push_back(clause);
        };
        dp::CnfReader::read_from_stream(stream, func);

        // Restore std::cerr to its original state
        std::cerr.rdbuf(original_cerr_buf);

        REQUIRE(clauses.size() == 2);
        std::string warning_output = captured_warnings.str();
        REQUIRE_FALSE(warning_output.empty());
        REQUIRE(warning_output.find("warning") != std::string::npos);
    }

    SECTION("Reading from a non-existent file throws exception") {
        REQUIRE_THROWS_AS(dp::CnfReader::read_from_file_to_vector("non_existent_file.cnf"), dp::CnfReader::failure);
    }
}
