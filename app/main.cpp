#include <vector>
#include <stdexcept>
#include <sylvan_obj.hpp>
#include <simple_logger.h>
#include "args_parser.hpp"
#include "data_structures/sylvan_zdd_cnf.hpp"
#include "algorithms/dp_elimination.hpp"
#include "algorithms/heuristics.hpp"
#include "metrics/dp_metrics.hpp"

using namespace dp;

class StopCondition {
public:
    explicit StopCondition(size_t orig_cnf_size, float max_growth) :
            m_max_size(static_cast<size_t>(static_cast<float>(orig_cnf_size) * max_growth)) {}

    bool operator()(size_t, const SylvanZddCnf &cnf, size_t cnf_size, const HeuristicResult &result) const {
        if (cnf.is_empty() || cnf.contains_empty()) {
            LOG_INFO << "Found empty formula or empty clause, stopping DP elimination";
            return true;
        } else if (!result.success) {
            LOG_INFO << "Didn't find variable to be eliminated, stopping DP elimination";
            return true;
        } else if (cnf_size > m_max_size) {
            LOG_INFO << "Formula grew too large (" << cnf_size << " > " << m_max_size << "), stopping DP elimination";
            return true;
        } else {
            return false;
        }
    }

private:
    const size_t m_max_size;
};

SylvanZddCnf eliminate_vars_select_heuristic(const SylvanZddCnf &cnf, const ArgsParser &args) {
    StopCondition stop_condition{cnf.count_clauses(), args.get_max_formula_growth()};
    if (args.get_heuristic() == ArgsParser::Heuristic::Simple) {
        LOG_INFO << "Using the Simple heuristic";
        heuristics::SimpleHeuristic heuristic;
        return eliminate_vars(cnf, heuristic, stop_condition, args.get_absorbed_clause_elimination_interval());
    } else if (args.get_heuristic() == ArgsParser::Heuristic::UnitLiteral) {
        LOG_INFO << "Using the UnitLiteral heuristic";
        heuristics::UnitLiteralHeuristic heuristic;
        return eliminate_vars(cnf, heuristic, stop_condition, args.get_absorbed_clause_elimination_interval());
    } else if (args.get_heuristic() == ArgsParser::Heuristic::ClearLiteral) {
        LOG_INFO << "Using the ClearLiteral heuristic";
        heuristics::ClearLiteralHeuristic heuristic;
        return eliminate_vars(cnf, heuristic, stop_condition, args.get_absorbed_clause_elimination_interval());
    } else if (args.get_heuristic() == ArgsParser::Heuristic::MinimalBloat) {
        LOG_INFO << "Using the MinimalBloat heuristic";
        heuristics::MinimalScoreHeuristic<heuristics::scores::bloat_score> heuristic{args.get_min_var(),
                                                                                     args.get_max_var()};
        return eliminate_vars(cnf, heuristic, stop_condition, args.get_absorbed_clause_elimination_interval());
    } else {
        throw std::logic_error("Heuristic not implemented");
    }
}

TASK_1(int, impl, const ArgsParser *, args_ptr)
{
    // Lace is a C framework, can't pass C++ arguments...
    const ArgsParser &args = *args_ptr;

    // initialize Sylvan
    Sylvan::initPackage(args.get_sylvan_table_size(),
                        args.get_sylvan_table_max_size(),
                        args.get_sylvan_cache_size(),
                        args.get_sylvan_cache_max_size());
    sylvan_init_zdd();

    // load input file
    std::string input_file_name = args.get_input_cnf_file_name();
    std::cout << "Reading input formula from file " << input_file_name << "..." << std::endl;
    SylvanZddCnf cnf = SylvanZddCnf::from_file(input_file_name);
    std::cout << "Input formula has " << cnf.count_clauses() << " clauses" << std::endl;

    // perform the algorithm
    std::cout << "Eliminating variables..." << std::endl;
    SylvanZddCnf result = eliminate_vars_select_heuristic(cnf, args);

    // write result to file
    std::string output_file_name = args.get_output_cnf_file_name();
    result.write_dimacs_to_file(output_file_name);
    std::cout << "Formula with " << result.count_clauses() << " clauses written to file " << output_file_name
              << std::endl;

    // export metrics
    std::string metrics_file_name = args.get_metrics_file_name();
    std::ofstream metrics_file{metrics_file_name};
    std::cout << "Exporting metrics to " << metrics_file_name << std::endl;
    metrics.export_json(metrics_file);

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

    // initialize logging
    if (!args->get_log_file_name().empty()) {
        simple_logger::Config::logFileName = args->get_log_file_name();
    }

    // initialize Lace
    size_t n_workers = args->get_lace_threads();
    size_t deque_size = 0; // default value for the size of task deques for the workers
    lace_start(n_workers, deque_size);
    LOG_INFO << "Lace started with " << lace_workers() << " threads";

    // run main Lace thread
    return RUN(impl, &args.value());

    // The lace_start command also exits Lace after _main is completed.
}
