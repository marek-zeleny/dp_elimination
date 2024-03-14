#include "args_parser.hpp"

#include <utility>
#include <CLI/CLI.hpp>

std::optional<ArgsParser> ArgsParser::parse(int argc, char *argv[]) {
    ArgsParser args;
    CLI::App app("Davis Putnam elimination algorithm for preprocessing CNF formulas");
    app.option_defaults()->always_capture_default();
    argv = app.ensure_utf8(argv);

    app.set_config("--config", "data/config.toml");

    app.add_option("-i,--input-file", args.m_input_cnf_file_name,
                   "File containing the input formula in DIMACS format")
                   ->required();
    app.add_option("-o,--output-file", args.m_output_cnf_file_name,
                   "File for writing the formula after variable elimination");
    app.add_option("-l,--log-file", args.m_log_file_name,
                   "File for writing logs");
    app.add_option("-e,--eliminate", args.m_eliminated_vars,
                   "Number of variables to eliminate");
    app.add_option("-a,--absorbed-clause-elimination-interval", args.m_absorbed_clause_elimination_interval,
                   "Number of eliminated variables before absorbed clauses are removed (never if 0)");
    app.add_option("-v,--var-range", args.m_var_range,
                   "Range of variables that are allowed to be eliminated");
    app.add_option("--sylvan-table-size", args.m_sylvan_table_size,
                   "Sylvan table size (default and max), should be a power of 2");
    app.add_option("--sylvan-cache-size", args.m_sylvan_cache_size,
                   "Sylvan cache size(default and max), should be a power of 2");

    try {
        app.parse(argc, argv);
        if (args.get_min_var() > args.get_max_var()) {
            throw CLI::ValidationError("--var-range", "Minimum variable to be eliminated cannot be larger than maximum variable");
        }
    } catch (const CLI::ParseError &e) {
        app.exit(e);
        return std::nullopt;
    }
    return std::move(args);
}
