#pragma once

#include <concepts>
#include <algorithm>
#include <limits>
#include <simple_logger.h>
#include "data_structures/sylvan_zdd_cnf.hpp"

namespace dp {

/**
 * Result of a search for literal to be eliminated.
 *
 * If the search was not successful, all fields except for `success` are undefined.
 */
struct HeuristicResult {
    using Score = int64_t;

    /**
     * Heuristic score given to the result.
     */
    Score score;
    /**
     * Literal selected by the heuristic.
     */
    SylvanZddCnf::Literal literal;
    /**
     * true if a suitable literal was found, otherwise false.
     */
    bool success;

    HeuristicResult(bool success, SylvanZddCnf::Literal literal, Score score) :
        score(score), literal(literal), success(success) {}
};

/**
 * Contains heuristics for selecting literals to be eliminated.
 */
namespace heuristics {

/**
 * Specifies a scoring function working with ZBDD variable statistics.
 */
template<typename F>
concept IsScoreEvaluator = requires(F f, const SylvanZddCnf::VariableStats &stats) {
    { f(stats) } -> std::convertible_to<HeuristicResult::Score>;
};

/**
 * Selects the root literal.
 *
 * Legacy heuristic not reachable from user interface.
 */
class SimpleHeuristic {
public:
    /**
     * @return The result is binary (does not provide any score).
     */
    HeuristicResult operator()(const SylvanZddCnf &cnf) {
        SylvanZddCnf::Literal l = cnf.get_root_literal();
        LOG_INFO << "Heuristic found root literal " << l;
        if (l == 0) {
            return {false, l, 0};
        } else {
            return {true, l, 0};
        }
    }
};

/**
 * Selects a unit literal if one exists, otherwise fails.
 *
 * Legacy heuristic not reachable from user interface.
 */
class UnitLiteralHeuristic {
public:
    /**
     * @return The result is binary (does not provide any score).
     */
    HeuristicResult operator()(const SylvanZddCnf &cnf) {
        SylvanZddCnf::Literal l = cnf.get_unit_literal();
        if (l == 0) {
            l = cnf.get_root_literal();
            LOG_INFO << "Heuristic didn't find any unit literal, returning root literal " << l << " instead";
            return {false, l, 0};
        } else {
            LOG_INFO << "Heuristic found unit literal " << l;
            return {true, l, 0};
        }
    }
};

/**
 * Selects a clean literal (with no complement in the formula) if one exists, otherwise fails.
 *
 * Legacy heuristic not reachable from user interface.
 */
class ClearLiteralHeuristic {
public:
    /**
     * @return The result is binary (does not provide any score).
     */
    HeuristicResult operator()(const SylvanZddCnf &cnf) {
        SylvanZddCnf::Literal l = cnf.get_clear_literal();
        if (l == 0) {
            l = cnf.get_root_literal();
            LOG_INFO << "Heuristic didn't find any clear literal, returning root literal " << l << " instead";
            return {false, l, 0};
        } else {
            LOG_INFO << "Heuristic found clear literal " << l;
            return {true, l, 0};
        }
    }
};

/**
 * Heuristic based on variable ordering. Selects the first variable in given order.
 *
 * Works on a specified variable range. If no variable within the range is found, it fails.
 *
 * @tparam Ascending If true, selects the smallest existing variable in range, otherwise selects the largest.
 */
template<bool Ascending>
class OrderHeuristic {
public:
    OrderHeuristic(size_t min_var, size_t max_var) : m_min_var(min_var), m_max_var(max_var) {}

    /**
     * @return The result is binary (does not provide any score).
     */
    HeuristicResult operator()(const SylvanZddCnf &cnf) {
        if (cnf.is_empty()) {
            LOG_INFO << "Heuristic called for an empty formula";
            return {false, 0, 0};
        }
        SylvanZddCnf::FormulaStats stats = cnf.find_all_literals();
        const size_t min_var = std::max(m_min_var, stats.index_shift);
        const size_t max_var = std::min(m_max_var, stats.vars.size() + stats.index_shift - 1);
        if (min_var <= max_var) {
            const size_t start = Ascending ? min_var : max_var;
            const size_t end = Ascending ? max_var + 1 : min_var - 1;
            for (size_t var = start; var != end; var = loop_update(var)) {
                size_t idx = var - stats.index_shift;
                const SylvanZddCnf::VariableStats &var_stats = stats.vars[idx];
                // skip missing variables
                if (var_stats.positive_clause_count == 0 && var_stats.negative_clause_count == 0) {
                    continue;
                }
                if constexpr (Ascending) {
                    LOG_INFO << "Heuristic found smallest variable " << var;
                } else {
                    LOG_INFO << "Heuristic found largest variable " << var;
                }
                return {true, static_cast<SylvanZddCnf::Literal>(var), 0};
            }
        }
        LOG_INFO << "Heuristic didn't find any variable in range";
        return {false, 0, 0};
    }

private:
    const size_t m_min_var;
    const size_t  m_max_var;

    static constexpr size_t loop_update(size_t x) {
        if constexpr (Ascending) {
            return x + 1;
        } else {
            return x - 1;
        }
    }
};

/**
 * Heuristic based on formula statistics. Selects a variable with minimal score.
 *
 * Works on a specified variable range. If no variable within the range is found, it fails.
 *
 * @tparam ScoreEvaluator Function providing a score for a variable based on its occurrence counts.
 */
template<auto ScoreEvaluator> requires IsScoreEvaluator<decltype(ScoreEvaluator)>
class MinimalScoreHeuristic {
public:
    MinimalScoreHeuristic(size_t min_var, size_t max_var) : m_min_var(min_var), m_max_var(max_var) {}

    /**
     * @return If successful, the selected variable's score is given.
     */
    HeuristicResult operator()(const SylvanZddCnf &cnf) {
        if (cnf.is_empty()) {
            LOG_INFO << "Heuristic called for an empty formula";
            return {false, 0, 0};
        }
        SylvanZddCnf::FormulaStats stats = cnf.count_all_literals();
        // minimize score
        Score best_score = std::numeric_limits<Score>::max();
        size_t best_var = 0;
        const size_t min_var = std::max(m_min_var, stats.index_shift);
        const size_t max_var = std::min(m_max_var, stats.vars.size() + stats.index_shift - 1);
        for (size_t var = min_var; var <= max_var; ++var) {
            size_t idx = var - stats.index_shift;
            const SylvanZddCnf::VariableStats &var_stats = stats.vars[idx];
            // skip missing variables
            if (var_stats.positive_clause_count == 0 && var_stats.negative_clause_count == 0) {
                continue;
            }
            Score score = ScoreEvaluator(var_stats);
            if (score < best_score) {
                best_score = score;
                best_var = var;
            }
        }
        if (best_var == 0) {
            LOG_INFO << "Heuristic didn't find any variable in range";
            return {false, 0, 0};
        } else {
            LOG_INFO << "Heuristic found variable " << best_var << " with score " << best_score;
            return {true, static_cast<SylvanZddCnf::Literal>(best_var), best_score};
        }
    }

private:
    using Score = HeuristicResult::Score;

    const size_t m_min_var;
    const size_t m_max_var;
};

/**
 * Contains scoring functions for MinimalScoreHeuristic.
 */
namespace scores {

/**
 * Computes the upper bound of how much the formula can grow if the given variable is eliminated.
 */
inline HeuristicResult::Score bloat_score(const SylvanZddCnf::VariableStats &stats) {
    using Score = HeuristicResult::Score;
    auto removed_clauses = static_cast<Score>(stats.positive_clause_count + stats.negative_clause_count);
    auto new_clauses = static_cast<Score>(stats.positive_clause_count * stats.negative_clause_count);
    return new_clauses - removed_clauses;
}

} // namespace scores

} // namespace heuristics

} // namespace dp
