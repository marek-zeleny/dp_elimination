#include "args_parser.hpp"

#include <utility>
#include <unordered_map>
#include <CLI/CLI.hpp>

// Help message prints them in reversed order
static const std::unordered_map<std::string, ArgsParser::Heuristic> heuristic_map {
        {"minimal_bloat", ArgsParser::Heuristic::MinimalBloat},
        {"clear_literal", ArgsParser::Heuristic::ClearLiteral},
        {"unit_literal", ArgsParser::Heuristic::UnitLiteral},
        {"simple", ArgsParser::Heuristic::Simple},
};

static const std::unordered_map<std::string, ArgsParser::AbsorbedRemovalAlgorithm> absorbed_removal_algorithm_map {
        {"watched_literals", ArgsParser::AbsorbedRemovalAlgorithm::WatchedLiterals},
        {"zbdd", ArgsParser::AbsorbedRemovalAlgorithm::ZBDD},
};

static const std::unordered_map<std::string, ArgsParser::Condition> condition_full_map {
        {"absolute_size", ArgsParser::Condition::AbsoluteSize},
        {"relative_size", ArgsParser::Condition::RelativeSize},
        {"interval", ArgsParser::Condition::Interval},
        {"never", ArgsParser::Condition::Never},
};

static const std::unordered_map<std::string, ArgsParser::Condition> condition_partial_map {
        {"relative_size", ArgsParser::Condition::RelativeSize},
        {"interval", ArgsParser::Condition::Interval},
        {"never", ArgsParser::Condition::Never},
};

std::optional<ArgsParser> ArgsParser::parse(int argc, char *argv[]) {
    ArgsParser args;
    CLI::App app("Davis Putnam elimination algorithm for preprocessing CNF formulas");
    app.get_formatter()->column_width(40);
    app.option_defaults()->always_capture_default(true);

    app.set_config("--config", "",
                   "Read a config file (.ini or .toml); precedence from the last if multiple")
                   ->expected(1, std::numeric_limits<int>::max())
                   ->multi_option_policy(CLI::MultiOptionPolicy::Reverse);
    app.allow_config_extras(CLI::config_extras_mode::error);

    // files
    app.add_option("input-file", args.m_input_cnf_file_name,
                   "File containing the input formula in DIMACS format")
                   ->group("Files")
                   ->required();
    app.add_option("-o,--output-file", args.m_output_cnf_file_name,
                   "File for writing the formula after variable elimination")
                   ->group("Files");
    app.add_option("-m,--metrics-file", args.m_metrics_file_name,
                   "File for exporting metrics (JSON)")
                   ->group("Files");
    app.add_option("-l,--log-file", args.m_log_file_name,
                   "File for writing logs")
                   ->group("Files");
    app.add_option("--output-max-size", args.m_output_cnf_file_max_size,
                   "Maximum size of output formula (# clauses); if larger, no output is written")
                   ->group("Files");

    // algorithm
    app.option_defaults()->always_capture_default(false);
    app.add_option("--heuristic", args.m_heuristic,
                   "Heuristic for selecting eliminated literals")
                   ->group("Algorithm")
                   ->required()
                   ->transform(CLI::CheckedTransformer(heuristic_map,
                                                       CLI::ignore_case,
                                                       CLI::ignore_space,
                                                       CLI::ignore_underscore));
    app.option_defaults()->always_capture_default(true);
    app.add_option("--absorbed-removal-algorithm", args.m_absorbed_removal_algorithm,
                   "Algorithm for removing absorbed clauses")
                   ->group("Algorithm")
                   ->transform(CLI::CheckedTransformer(absorbed_removal_algorithm_map,
                                                       CLI::ignore_case,
                                                       CLI::ignore_space,
                                                       CLI::ignore_underscore));

    // complete minimization
    app.add_option("--complete-minimization-condition", args.m_complete_minimization_condition,
                   "Condition on when to fully minimize the formula")
                   ->group("Complete minimization")
                   ->transform(CLI::CheckedTransformer(condition_partial_map,
                                                       CLI::ignore_case,
                                                       CLI::ignore_space,
                                                       CLI::ignore_underscore));
    app.add_option("--complete-minimization-interval", args.m_complete_minimization_interval,
                   "Number of eliminated variables before complete minimization of the formula"
                   "\nneeds --complete-minimization-condition=interval")
                   ->group("Complete minimization")
                   ->check(CLI::Range(1ul, std::numeric_limits<size_t>::max()));
    app.add_option("--complete-minimization-relative-size", args.m_complete_minimization_relative_size,
                   "Relative growth of formula before complete minimization (must be larger than 1)"
                   "\nneeds --complete-minimization-condition=relative_size")
                   ->group("Complete minimization")
                   ->check(CLI::Range(1.0f, 1000.0f));

    // incremental minimization
    app.add_option("--incremental-minimization-condition", args.m_incremental_minimization_condition,
                   "Condition on when to incrementally (partially) minimize the formula")
                   ->group("Incremental minimization")
                   ->transform(CLI::CheckedTransformer(condition_full_map,
                                                       CLI::ignore_case,
                                                       CLI::ignore_space,
                                                       CLI::ignore_underscore));
    app.add_option("--incremental-minimization-interval", args.m_incremental_minimization_interval,
                   "Number of eliminated variables before incremental minimization of the formula"
                   "\nneeds --incremental-minimization-condition=interval")
                   ->group("Incremental minimization")
                   ->check(CLI::Range(1ul, std::numeric_limits<size_t>::max()));
    app.add_option("--incremental-minimization-relative-size", args.m_incremental_minimization_relative_size,
                   "Relative size of added formula compared to the base formula in order to trigger incremental"
                   "\nminimization when computing their union (must be larger than 0)"
                   "\nneeds --incremental-minimization-condition=relative_size")
                   ->group("Incremental minimization")
                   ->check(CLI::Range(0.0f, 1000.0f));
    app.add_option("--incremental-minimization-absolute_size", args.m_incremental_minimization_absolute_size,
                   "Absolute size of added formula in order to trigger incremental minimization when computing its"
                   "\nunion with the base formula"
                   "\nneeds --incremental-minimization-condition=absolute_size")
                   ->group("Incremental minimization")
                   ->check(CLI::Range(0ul, std::numeric_limits<size_t>::max()));

    // subsumed removal
    app.add_option("--subsumed-removal-condition", args.m_subsumed_removal_condition,
                   "Condition on when to remove subsumed clauses from the formula")
                   ->group("Subsumed removal")
                   ->transform(CLI::CheckedTransformer(condition_full_map,
                                                       CLI::ignore_case,
                                                       CLI::ignore_space,
                                                       CLI::ignore_underscore));
    app.add_option("--subsumed-removal-interval", args.m_subsumed_removal_interval,
                   "Number of eliminated variables before removing subsumed clauses"
                   "\nneeds --subsumed-removal-condition=interval")
                   ->group("Subsumed removal")
                   ->check(CLI::Range(1ul, std::numeric_limits<size_t>::max()));
    app.add_option("--subsumed-removal-relative-size", args.m_subsumed_removal_relative_size,
                   "Relative size of added formula compared to the base formula in order to trigger subsumed clause"
                   "\nremoval (must be larger than 0)"
                   "\nneeds --subsumed-removal-condition=relative_size")
                   ->group("Subsumed removal")
                   ->check(CLI::Range(0.0f, 1000.0f));
    app.add_option("--subsumed-removal-absolute_size", args.m_subsumed_removal_absolute_size,
                   "Absolute size of added formula in order to trigger subsumed clause removal when computing its"
                   "\nunion with the base formula"
                   "\nneeds --subsumed-removal-condition=absolute_size")
                   ->group("Subsumed removal")
                   ->check(CLI::Range(0ul, std::numeric_limits<size_t>::max()));

    // stop conditions
    app.add_option("-i,--max-iterations", args.m_max_iterations,
                   "Maximum number of iterations before stopping")
                   ->group("Stop conditions");
    app.add_option("-t,--timeout,--max-duration,--max-duration-seconds", args.m_max_duration_seconds,
                   "Maximum duration (timeout) in seconds (can overshoot, waits until iteration ends)")
                   ->group("Stop conditions");
    app.add_option("-g,--max-formula-growth", args.m_max_formula_growth,
                   "Maximum allowed growth of the number of clauses relative to the input formula")
                   ->group("Stop conditions");
    app.add_option("-v,--var-range", args.m_var_range,
                   "Range of variables that are allowed to be eliminated")
                   ->group("Stop conditions");

    // sylvan
    app.add_option("--sylvan-table-size", args.m_sylvan_table_size_pow,
                   "Sylvan table size (default and max) as a base-2 logarithm (20 -> 24 MB)")
                   ->group("Sylvan");
    app.add_option("--sylvan-cache-size", args.m_sylvan_cache_size_pow,
                   "Sylvan cache size (default and max) as a base-2 logarithm (20 -> 36 MB)")
                   ->group("Sylvan");
    app.add_option("--lace-threads", args.m_lace_threads,
                   "Number of lace threads (0 for auto-detect)")
                   ->group("Sylvan");

    try {
        app.parse(argc, argv);
        args.m_config_string = app.config_to_str(true, false);
        if (args.get_min_var() > args.get_max_var()) {
            throw CLI::ValidationError("--var-range",
                                       "Minimum variable to be eliminated (" +
                                       std::to_string(args.get_min_var()) +
                                       ") cannot be larger than maximum variable (" +
                                       std::to_string(args.get_max_var()) +
                                       ")");
        }
    } catch (const CLI::ParseError &e) {
        app.exit(e);
        return std::nullopt;
    }
    return std::move(args);
}
