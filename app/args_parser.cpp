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

    app.add_option("-i,--input-file", args.m_input_cnf_file_name,
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
    app.add_option("-g,--max-formula-growth", args.m_max_formula_growth,
                   "Maximum allowed growth of the number of clauses relative to the input formula")
                   ->group("Algorithm");
    app.add_option("-a,--absorbed-clause-elimination-interval", args.m_absorbed_clause_elimination_interval,
                   "Number of eliminated variables before absorbed clauses are removed (never if 0)")
                   ->group("Algorithm");
    app.add_option("-v,--var-range", args.m_var_range,
                   "Range of variables that are allowed to be eliminated")
                   ->group("Algorithm");

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
