#pragma once

#include <concepts>
#include <algorithm>
#include <limits>
#include <simple_logger.h>
#include "data_structures/sylvan_zdd_cnf.hpp"

namespace dp {

struct HeuristicResult {
    using Score = int64_t;

    Score score;
    SylvanZddCnf::Literal literal;
    bool success;

    HeuristicResult(bool success, SylvanZddCnf::Literal literal, Score score) :
        score(score), literal(literal), success(success) {}
};

template<typename F>
concept IsHeuristic = requires(F f, const SylvanZddCnf &cnf) {
    { f(cnf) } -> std::same_as<HeuristicResult>;
};

template<typename F>
concept IsScoreEvaluator = requires(F f, const SylvanZddCnf::VariableStats &stats) {
    { f(stats) } -> std::convertible_to<HeuristicResult::Score>;
};

namespace heuristics {

class SimpleHeuristic {
public:
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

class UnitLiteralHeuristic {
public:
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

class ClearLiteralHeuristic {
public:
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

template<auto ScoreEvaluator> requires IsScoreEvaluator<decltype(ScoreEvaluator)>
class MinimalScoreHeuristic {
public:
    MinimalScoreHeuristic(size_t min_var, size_t max_var) : m_min_var(min_var), m_max_var(max_var) {}

    HeuristicResult operator()(const SylvanZddCnf &cnf) {
        SylvanZddCnf::FormulaStats stats = cnf.get_formula_statistics();
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

namespace scores {

inline HeuristicResult::Score bloat_score(const SylvanZddCnf::VariableStats &stats) {
    using Score = HeuristicResult::Score;
    auto removed_clauses = static_cast<Score>(stats.positive_clause_count + stats.negative_clause_count);
    auto new_clauses = static_cast<Score>(stats.positive_clause_count * stats.negative_clause_count);
    return new_clauses - removed_clauses;
}

} // namespace scores

} // namespace heuristics

} // namespace dp
