#include <vector>
#include <chrono>
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

class StopCondition {
private:
    using clock = std::chrono::steady_clock;

public:
    StopCondition(size_t orig_cnf_size, const std::optional<size_t> &max_iterations,
                  const std::optional<float> &max_growth, const std::optional<size_t> &max_duration_seconds) :
            m_max_iterations(max_iterations), m_max_size(get_max_size(orig_cnf_size, max_growth)),
            m_max_end_time(get_max_end_time(max_duration_seconds)) {}

    bool operator()(size_t iter, const SylvanZddCnf &cnf, size_t cnf_size, const HeuristicResult &result) const {
        if (cnf.is_empty() || cnf.contains_empty()) {
            LOG_INFO << "Found empty formula or empty clause, stopping DP elimination";
            return true;
        } else if (!result.success) {
            LOG_INFO << "Didn't find variable to be eliminated, stopping DP elimination";
            return true;
        } else if (m_max_iterations.has_value() && iter > m_max_iterations) {
            LOG_INFO << "Maximum number of iterations reached (" << iter << "), stopping DP elimination";
            return true;
        } else if (m_max_size.has_value() && cnf_size > m_max_size) {
            LOG_INFO << "Formula grew too large (" << cnf_size << " > " << *m_max_size << "), stopping DP elimination";
            return true;
        } else if (m_max_end_time.has_value() && clock::now() > m_max_end_time) {
            LOG_INFO << "Maximum duration time reached, stopping DP elimination";
            return true;
        } else {
            return false;
        }
    }

private:
    const std::optional<size_t> m_max_iterations;
    const std::optional<size_t> m_max_size;
    const std::optional<clock::time_point> m_max_end_time;

    static std::optional<size_t> get_max_size(size_t orig_cnf_size, const std::optional<float> &max_growth) {
        if (max_growth.has_value()) {
            return static_cast<size_t>(static_cast<float>(orig_cnf_size) * max_growth.value());
        } else {
            return std::nullopt;
        }
    }

    static std::optional<clock::time_point> get_max_end_time(const std::optional<size_t> &max_duration_seconds) {
        if (max_duration_seconds.has_value()) {
            return clock::now() + std::chrono::seconds(max_duration_seconds.value());
        } else {
            return std::nullopt;
        }
    }
};

bool never_absorbed_condition(size_t, size_t, size_t) {
    return false;
}

class IntervalAbsorbedCondition {
public:
    explicit IntervalAbsorbedCondition(size_t interval) : m_interval(interval) {}

    bool operator()(size_t iter, size_t, size_t) const {
        return iter % m_interval == m_interval - 1;
    }

private:
    const size_t m_interval;
};

class GrowthAbsorbedCondition {
public:
    explicit GrowthAbsorbedCondition(float max_growth) : m_max_growth(max_growth) {}

    bool operator()(size_t, size_t prev_size, size_t size) const {
        return static_cast<float>(size) > static_cast<float>(prev_size) * m_max_growth;
    }

private:
    const float m_max_growth;
};

class AllowedVariablePredicate {
public:
    AllowedVariablePredicate(size_t min_var, size_t max_var) : m_min_var(min_var), m_max_var(max_var) {}

    bool operator()(const size_t &var) {
        return m_min_var <= var && var <= m_max_var;
    }

private:
    const size_t m_min_var;
    const size_t m_max_var;
};

EliminationAlgorithmConfig create_config_from_args(const SylvanZddCnf &cnf, const ArgsParser &args) {
    EliminationAlgorithmConfig config;
    config.stop_condition = StopCondition(cnf.count_clauses(), args.get_max_iterations(),
                                          args.get_max_formula_growth(), args.get_max_duration_seconds());
    config.is_allowed_variable = AllowedVariablePredicate(args.get_min_var(), args.get_max_var());

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
            //config.unify_with_non_absorbed = absorbed_clause_detection::remove_absorbed_clauses_without_conversion;
            //break;
            throw std::logic_error("Absorbed removal algorithm not implemented");
        case ArgsParser::AbsorbedRemovalAlgorithm::WatchedLiterals:
            config.remove_absorbed_clauses = absorbed_clause_detection::with_conversion::remove_absorbed_clauses;
            config.unify_with_non_absorbed = absorbed_clause_detection::with_conversion::unify_with_non_absorbed;
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
    SylvanZddCnf::hook_sylvan_gc_log();

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

void set_lace_stack_limit(size_t size = 0) {
    rlimit limit{size, size};
    // if a size is given, set the limit manually
    if (size > 0) {
        setrlimit(RLIMIT_STACK, &limit);
    }
    // get stack limit and give it to Lace
    if (getrlimit(RLIMIT_STACK, &limit) == 0) {
        lace_set_stacksize(limit.rlim_cur);
        LOG_INFO << "Stack limit: " << limit.rlim_cur << " / " << limit.rlim_max;
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
    LOG_INFO << "Used configuration:\n" << args->get_config_string();

    // initialize Lace
    set_lace_stack_limit();
    size_t n_workers = args->get_lace_threads();
    size_t deque_size = 0; // default value for the size of task deques for the workers
    lace_start(n_workers, deque_size);
    LOG_INFO << "Lace started with " << lace_workers() << " threads";

    // run main Lace thread
    return RUN(impl, &args.value());

    // The lace_start command also exits Lace after _main is completed.
}
