#include "args_parser.hpp"

#include <utility>
#include <CLI/CLI.hpp>

std::optional<ArgsParser> ArgsParser::parse(int argc, char *argv[]) {
    ArgsParser args;
    CLI::App app("Davis Putnam elimination algorithm for preprocessing CNF formulas");
    app.option_defaults()->always_capture_default();
    argv = app.ensure_utf8(argv);

    app.add_option("-i,--input-file", args.m_input_cnf_file_name,
                   "File containing the input formula in DIMACS format")
                   ->required();
    app.add_option("-o,--output-file", args.m_output_cnf_file_name,
                   "File for writing the formula after variable elimination ");
    app.add_option("-l,--log-file", args.m_log_file_name,
                   "File for writing logs");
    app.add_option("-e,--eliminate", args.m_eliminated_vars,
                   "Number of variables to eliminate");

    try {
        app.parse(argc, argv);
    } catch (const CLI::ParseError &e) {
        app.exit(e);
        return std::nullopt;
    }
    return std::move(args);
}
