#include <vector>
#include <stdexcept>
#include <sys/resource.h>
#include <sylvan_obj.hpp>
#include <simple_logger.h>
#include "args_parser.hpp"
#include "data_structures/sylvan_zdd_cnf.hpp"
#include "algorithms/dp_elimination.hpp"
#include "algorithms/unit_propagation.hpp"
#include "algorithms/heuristics.hpp"
#include "metrics/dp_metrics.hpp"

using namespace dp;

bool generic_stop_condition(size_t, const SylvanZddCnf &cnf, size_t, const HeuristicResult &result) {
    if (cnf.is_empty() || cnf.contains_empty()) {
        LOG_INFO << "Found empty formula or empty clause, stopping DP elimination";
        return true;
    } else if (!result.success) {
        LOG_INFO << "Didn't find variable to be eliminated, stopping DP elimination";
        return true;
    } else {
        return false;
    }
}

class GrowthStopCondition {
public:
    explicit GrowthStopCondition(size_t orig_cnf_size, float max_growth) :
            m_max_size(static_cast<size_t>(static_cast<float>(orig_cnf_size) * max_growth)) {}

    bool operator()(size_t iter, const SylvanZddCnf &cnf, size_t cnf_size, const HeuristicResult &result) const {
        if (generic_stop_condition(iter, cnf, cnf_size, result)) {
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

bool never_absorbed_condition(bool, size_t, size_t) {
    return false;
}

class IntervalAbsorbedCondition {
public:
    explicit IntervalAbsorbedCondition(size_t interval) : m_interval(interval) {}

    bool operator()(bool main_loop, size_t iter, size_t) const {
        if (main_loop) {
            return iter % m_interval == m_interval - 1;
        } else {
            return iter % m_interval != m_interval - 1;
        }
    }

private:
    const size_t m_interval;
};

class GrowthAbsorbedCondition {
public:
    explicit GrowthAbsorbedCondition(float max_growth) : m_max_growth(max_growth) {}

    bool operator()(bool main_loop, size_t iter, size_t size) {
        if (impl(main_loop, iter, size)) {
            m_prev_size = size;
            return true;
        } else {
            return false;
        }
    }

private:
    const float m_max_growth;
    size_t m_prev_size{0};

    bool impl(bool main_loop, size_t, size_t size) const {
        if (main_loop) {
            return static_cast<float>(size) > static_cast<float>(m_prev_size) * m_max_growth;
        } else {
            return size > m_prev_size;
        }
    }
};

EliminationAlgorithmConfig create_config_from_args(const SylvanZddCnf &cnf, const ArgsParser &args) {
    EliminationAlgorithmConfig config;

    switch (args.get_heuristic()) {
        case ArgsParser::Heuristic::Simple:
            LOG_INFO << "Using the Simple heuristic";
            config.heuristic = heuristics::SimpleHeuristic();
            break;
        case ArgsParser::Heuristic::UnitLiteral:
            LOG_INFO << "Using the UnitLiteral heuristic";
            config.heuristic = heuristics::UnitLiteralHeuristic();
            break;
        case ArgsParser::Heuristic::ClearLiteral:
            LOG_INFO << "Using the ClearLiteral heuristic";
            config.heuristic = heuristics::ClearLiteralHeuristic();
            break;
        case ArgsParser::Heuristic::MinimalBloat:
            LOG_INFO << "Using the MinimalBloat heuristic";
            config.heuristic = heuristics::MinimalScoreHeuristic<heuristics::scores::bloat_score>(args.get_min_var(),
                                                                                                  args.get_max_var());
            break;
        default:
            throw std::logic_error("Heuristic not implemented");
    }

    switch (args.get_stop_condition()) {
        case ArgsParser::StopCondition::Growth:
            config.stop_condition = GrowthStopCondition(cnf.count_clauses(), args.get_max_formula_growth());
            break;
        case ArgsParser::StopCondition::AllVariables:
            config.stop_condition = generic_stop_condition;
            break;
        default:
            throw std::logic_error("Stop condition not implemented");
    }

    switch (args.get_absorbed_removal_condition()) {
        case ArgsParser::AbsorbedRemovalCondition::Never:
            config.remove_absorbed_condition = never_absorbed_condition;
            break;
        case ArgsParser::AbsorbedRemovalCondition::Interval:
            config.remove_absorbed_condition = IntervalAbsorbedCondition(args.get_absorbed_removal_interval());
            break;
        case ArgsParser::AbsorbedRemovalCondition::FormulaGrowth:
            config.remove_absorbed_condition = GrowthAbsorbedCondition(args.get_absorbed_removal_growth());
            break;
        default:
            throw std::logic_error("Absorbed removal condition not implemented");
    }

    switch (args.get_absorbed_removal_algorithm()) {
        case ArgsParser::AbsorbedRemovalAlgorithm::ZBDD:
            config.remove_absorbed_clauses = absorbed_clause_detection::remove_absorbed_clauses_without_conversion;
            break;
        case ArgsParser::AbsorbedRemovalAlgorithm::WatchedLiterals:
            config.remove_absorbed_clauses = absorbed_clause_detection::remove_absorbed_clauses_with_conversion;
            break;
        default:
            throw std::logic_error("Absorbed removal algorithm not implemented");
    }

    return config;
}

TASK_1(int, impl, const ArgsParser *, args_ptr)
{
    // Lace is a C framework, can't pass C++ arguments...
    const ArgsParser &args = *args_ptr;

    // initialize Sylvan
    sylvan::Sylvan::initPackage(args.get_sylvan_table_size(),
                        args.get_sylvan_table_max_size(),
                        args.get_sylvan_cache_size(),
                        args.get_sylvan_cache_max_size());
    sylvan::sylvan_init_zdd();

    // load input file
    std::string input_file_name = args.get_input_cnf_file_name();
    std::cout << "Reading input formula from file " << input_file_name << "..." << std::endl;
    SylvanZddCnf cnf = SylvanZddCnf::from_file(input_file_name);
    std::cout << "Input formula has " << cnf.count_clauses() << " clauses" << std::endl;

    // perform the algorithm
    std::cout << "Eliminating variables..." << std::endl;
    EliminationAlgorithmConfig config = create_config_from_args(cnf, args);
    SylvanZddCnf result = eliminate_vars(cnf, config);

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
    sylvan::Sylvan::quitPackage();
    return 0;
}

void set_stack_limit(size_t size) {
    struct rlimit limit;
    limit.rlim_cur = size;
    limit.rlim_max = size;
    setrlimit(RLIMIT_STACK, &limit);
    if (getrlimit(RLIMIT_STACK, &limit) == 0) {
        std::cout << "Stack limit: " << limit.rlim_cur << " / " << limit.rlim_max << std::endl;
    } else {
        throw std::runtime_error("Couldn't obtain stack limit");
    }
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
