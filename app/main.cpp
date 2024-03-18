#include <vector>
#include <sylvan_obj.hpp>
#include <simple_logger.h>
#include "args_parser.hpp"
#include "data_structures/sylvan_zdd_cnf.hpp"
#include "algorithms/dp_elimination.hpp"
#include "algorithms/heuristics.hpp"

TASK_1(int, impl, const ArgsParser *, args_ptr)
{
    using namespace dp;

    // Lace is a C framework, can't pass C++ arguments...
    const ArgsParser &args = *args_ptr;

    // initialize logging
    if (!args.get_log_file_name().empty()) {
        simple_logger::Config::logFileName = args.get_log_file_name();
    }

    // initialize Sylvan
    Sylvan::initPackage(args.get_sylvan_table_size(),
                        args.get_sylvan_table_max_size(),
                        args.get_sylvan_cache_size(),
                        args.get_sylvan_cache_max_size());
    sylvan_init_zdd();

    // load input file
    SylvanZddCnf cnf = SylvanZddCnf::from_file(args.get_input_cnf_file_name());
    std::cout << "Input formula has " << cnf.clauses_count() << " clauses" << std::endl;

    // perform the algorithm
    MinimalScoreHeuristic<bloat_score> heuristic{args.get_min_var(), args.get_max_var()};
    auto stop_condition = [](size_t, const SylvanZddCnf &cnf, const HeuristicResult &result) {
        if (cnf.is_empty() || cnf.contains_empty()) {
            LOG_INFO << "Found empty formula or empty clause, stopping DP elimination";
            return true;
        } else if (!result.success) {
            LOG_INFO << "Didn't find variable to be eliminated, stopping DP elimination";
            return true;
        } else if (result.score > 0) {
            LOG_INFO << "Heuristic with too high score, stopping DP elimination";
            return true;
        } else {
            return false;
        }
    };
    std::cout << "Eliminating variables..." << std::endl;
    SylvanZddCnf result = eliminate_vars(cnf, heuristic, stop_condition,
                                         args.get_absorbed_clause_elimination_interval());

    // write result to file
    std::string file_name = args.get_output_cnf_file_name();
    result.write_dimacs_to_file(file_name);
    std::cout << "Formula with " << result.clauses_count() << " clauses written to file " << file_name << std::endl;

    // quit sylvan, free memory
    Sylvan::quitPackage();
    return 0;
}

int main(int argc, char *argv[])
{
    // parse args
    std::optional<ArgsParser> args = ArgsParser::parse(argc, argv);
    if (!args.has_value()) {
        return 1;
    }

    // initialize Lace
    int n_workers = 0; // automatically detect number of workers
    size_t deque_size = 0; // default value for the size of task deques for the workers
    lace_start(n_workers, deque_size);

    // run main Lace thread
    return RUN(impl, &args.value());

    // The lace_start command also exits Lace after _main is completed.
}
