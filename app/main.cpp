#include <chrono>
#include <thread>
#include <stdexcept>
#include <sylvan_obj.hpp>
#include <simple_logger.h>
#include "args_parser.hpp"
#include "data_structures/sylvan_zdd_cnf.hpp"
#include "algorithms/dp_elimination.hpp"
#include "algorithms/unit_propagation.hpp"
#include "algorithms/heuristics.hpp"
#include "metrics/dp_metrics.hpp"

using namespace dp;

/**
 * Functor class for stop condition of DP elimination.
 */
class StopCondition {
private:
    using clock = std::chrono::steady_clock;

public:
    /**
     * Initializes a stop condition.
     *
     * @param orig_cnf_size Size of the input formula.
     * @param max_iterations If not nullopt, stops at most after the given number of iterations.
     * @param max_growth If not nullopt, stops when the formula becomes larger than the given limit.
     * @param max_duration_seconds If not nullopt, stops at most after the given number of seconds.
     *                             Note that this condition overshoots, and in certain situations (very large formula)
     *                             can overshoot by a significant amount of time.
     */
    StopCondition(size_t orig_cnf_size, const std::optional<size_t> &max_iterations,
                  const std::optional<float> &max_growth, const std::optional<size_t> &max_duration_seconds) :
            m_max_iterations(max_iterations), m_max_size(get_max_size(orig_cnf_size, max_growth)),
            m_max_end_time(get_max_end_time(max_duration_seconds)) {}

    /**
     * @param iteration Current iteration of the algorithm.
     * @param cnf Current state of the formula.
     * @param cnf_size Size of the formula (optimization).
     * @param result Result of the last query to literal selection heuristic.
     * @return true if DP elimination can continue, otherwise false.
     */
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

/**
 * Condition that is never true.
 */
bool never_condition(size_t, size_t, size_t) {
    return false;
}

/**
 * Condition that is true in certain intervals.
 */
class IntervalCondition {
public:
    explicit IntervalCondition(size_t interval) : m_interval(interval) {}

    bool operator()(size_t iter, size_t, size_t) const {
        return iter % m_interval == m_interval - 1;
    }

private:
    const size_t m_interval;
};

/**
 * Condition that is true if the second size is at least the given ratio of the first size.
 */
class RelativeSizeCondition {
public:
    explicit RelativeSizeCondition(float ratio) : m_ratio(ratio) {}

    bool operator()(size_t, size_t size1, size_t size2) const {
        return static_cast<float>(size2) > static_cast<float>(size1) * m_ratio;
    }

private:
    const float m_ratio;
};

/**
 * Condition that is true if the second size is larger than given threshold.
 */
class AbsoluteSizeCondition {
public:
    explicit AbsoluteSizeCondition(size_t max_size) : m_max_size(max_size) {}

    bool operator()(size_t, size_t, size_t size) const {
        return size > m_max_size;
    }

private:
    const size_t m_max_size;
};

/**
 * Predicate defining allowed range of variables.
 */
class AllowedVariablePredicate {
public:
    AllowedVariablePredicate(size_t min_var, size_t max_var) : m_min_var(min_var), m_max_var(max_var) {}

    bool operator()(const size_t &var) const {
        return m_min_var <= var && var <= m_max_var;
    }

private:
    const size_t m_min_var;
    const size_t m_max_var;
};

/**
 * Creates a DP elimination configuration based on CLI arguments.
 */
EliminationAlgorithmConfig create_config_from_args(const SylvanZddCnf &cnf, const ArgsParser &args) {
    EliminationAlgorithmConfig config;
    config.complete_minimization = absorbed_clause_detection::with_conversion::remove_absorbed_clauses;
    config.unify_and_remove_absorbed = absorbed_clause_detection::with_conversion::unify_with_non_absorbed;
    config.stop_condition = StopCondition(cnf.count_clauses(), args.get_max_iterations(),
                                          args.get_max_formula_growth(), args.get_max_duration_seconds());
    config.is_allowed_variable = AllowedVariablePredicate(args.get_min_var(), args.get_max_var());
    metrics.increase_counter(MetricsCounters::MinVar, static_cast<long>(args.get_min_var()));
    metrics.increase_counter(MetricsCounters::MaxVar, static_cast<long>(args.get_max_var()));

    switch (args.get_heuristic()) {
        case ArgsParser::Heuristic::Ascending:
            LOG_INFO << "Using the Ascending order heuristic";
            config.heuristic = heuristics::OrderHeuristic<true>(args.get_min_var(), args.get_max_var());
            break;
        case ArgsParser::Heuristic::Descending:
            LOG_INFO << "Using the Descending order heuristic";
            config.heuristic = heuristics::OrderHeuristic<false>(args.get_min_var(), args.get_max_var());
            break;
        case ArgsParser::Heuristic::MinimalBloat:
            LOG_INFO << "Using the MinimalBloat heuristic";
            config.heuristic = heuristics::MinimalScoreHeuristic<heuristics::scores::bloat_score>(args.get_min_var(),
                                                                                                  args.get_max_var());
            break;
        default:
            throw std::logic_error("Heuristic not implemented");
    }

    switch (args.get_complete_minimization_condition()) {
        case ArgsParser::Condition::Never:
            config.complete_minimization_condition = never_condition;
            break;
        case ArgsParser::Condition::Interval:
            config.complete_minimization_condition = IntervalCondition(args.get_complete_minimization_interval());
            break;
        case ArgsParser::Condition::RelativeSize:
            config.complete_minimization_condition = RelativeSizeCondition(
                    args.get_complete_minimization_relative_size());
            break;
        default:
            throw std::logic_error("Complete minimization condition not implemented");
    }

    switch (args.get_partial_minimization_condition()) {
        case ArgsParser::Condition::Never:
            config.partial_minimization_condition = never_condition;
            break;
        case ArgsParser::Condition::Interval:
            config.partial_minimization_condition = IntervalCondition(args.get_partial_minimization_interval());
            break;
        case ArgsParser::Condition::RelativeSize:
            config.partial_minimization_condition = RelativeSizeCondition(args.get_partial_minimization_relative_size());
            break;
        case ArgsParser::Condition::AbsoluteSize:
            config.partial_minimization_condition = AbsoluteSizeCondition(args.get_partial_minimization_absolute_size());
            break;
        default:
            throw std::logic_error("Partial minimization condition not implemented");
    }

    switch (args.get_incremental_absorption_removal_condition()) {
        case ArgsParser::Condition::Never:
            config.incremental_absorption_removal_condition = never_condition;
            break;
        case ArgsParser::Condition::Interval:
            config.incremental_absorption_removal_condition = IntervalCondition(
                    args.get_incremental_absorption_removal_interval());
            break;
        case ArgsParser::Condition::RelativeSize:
            config.incremental_absorption_removal_condition = RelativeSizeCondition(
                    args.get_incremental_absorption_removal_relative_size());
            break;
        case ArgsParser::Condition::AbsoluteSize:
            config.incremental_absorption_removal_condition = AbsoluteSizeCondition(
                    args.get_incremental_absorption_removal_absolute_size());
            break;
        default:
            throw std::logic_error("Incremental absorption removal condition not implemented");
    }

    return config;
}

/**
 * Implementation of the program.
 *
 * Initializes the Sylvan library, loads input file, performs DP elimination, exports resulting formula and metrics.
 * The function is separated from main() because historically it was a Lace task.
 */
int impl(const ArgsParser &args) {
    // initialize Sylvan
    LOG_INFO << "Initializing Sylvan";
    lace_resume();
    sylvan::Sylvan::initPackage(args.get_sylvan_table_size(),
                                args.get_sylvan_table_max_size(),
                                args.get_sylvan_cache_size(),
                                args.get_sylvan_cache_max_size());
    sylvan::sylvan_init_zdd();
    // avoid Lace's race condition
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    lace_suspend();
    LOG_DEBUG << "Sylvan initialized";
    SylvanZddCnf::hook_sylvan_gc_log();

    // load input file
    std::string input_file_name = args.get_input_cnf_file_name();
    std::cout << "Reading input formula from file " << input_file_name << "..." << std::endl;
    SylvanZddCnf cnf = SylvanZddCnf::from_file(input_file_name);
    std::cout << "Input formula has " << cnf.count_clauses() << " clauses" << std::endl;

    // perform the algorithm
    std::cout << "Eliminating variables..." << std::endl;
    EliminationAlgorithmConfig config = create_config_from_args(cnf, args);
    bool success = false;
    SylvanZddCnf result;
    try {
        result = eliminate_vars(cnf, config);
        success = true;
    } catch (const SylvanFullTableException &e) {
        LOG_ERROR << e.what();
        // export metrics - the program will terminate after exiting the catch block
        std::string metrics_file_name = args.get_metrics_file_name();
        std::ofstream metrics_file{metrics_file_name};
        std::cout << "Exporting metrics to " << metrics_file_name << std::endl;
        metrics.export_json(metrics_file);
    }

    // write result to file
    if (!success) {
        std::cout << "Elimination failed, see log for more information" << std::endl;
    } else if (result.count_clauses() > args.get_output_cnf_file_max_size()) {
        LOG_INFO << "Result is too large (" << result.count_clauses() << " > " << args.get_output_cnf_file_max_size()
                 << "), skipping writing it to an output file";
    } else {
        std::string output_file_name = args.get_output_cnf_file_name();
        result.write_dimacs_to_file(output_file_name);
        std::cout << "Formula with " << result.count_clauses() << " clauses written to file " << output_file_name
                  << std::endl;
    }

    // export metrics
    std::string metrics_file_name = args.get_metrics_file_name();
    std::ofstream metrics_file{metrics_file_name};
    std::cout << "Exporting metrics to " << metrics_file_name << std::endl;
    metrics.export_json(metrics_file);

    // quit sylvan, free memory
    LOG_INFO << "Quitting Sylvan";
    lace_resume();
    sylvan::Sylvan::quitPackage();
    // avoid Lace's race condition
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    lace_suspend();
    LOG_DEBUG << "Sylvan successfully exited";
    return 0;
}

int main(int argc, char *argv[]) {
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
    size_t n_workers = args->get_lace_threads();
    size_t deque_size = 0; // default value for the size of task deques for the workers
    lace_start(n_workers, deque_size);
    // Lace has a race condition when suspend() is called right after start() or resume(); avoid it by waiting for a bit
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    LOG_INFO << "Lace started with " << lace_workers() << " threads";
    lace_suspend();

    // run implementation
    int ret_val = impl(args.value());

    // again avoid Lace's race condition
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    lace_resume();
    lace_stop();
    return ret_val;
}
