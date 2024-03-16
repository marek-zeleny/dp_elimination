#pragma once

#include <type_traits>
#include <algorithm>
#include <limits>
#include <simple_logger.h>
#include "data_structures/sylvan_zdd_cnf.hpp"

namespace dp {

struct HeuristicResult {
    using Score = int64_t;

    const Score score;
    const SylvanZddCnf::Literal literal;
    const bool success;

    HeuristicResult(bool success, SylvanZddCnf::Literal literal, Score score) :
        score(score), literal(literal), success(success) {}
};

template<typename T>
concept IsHeuristic = requires(T t, const SylvanZddCnf &cnf) {
    { t(cnf) } -> std::same_as<HeuristicResult>;
};

class SimpleHeuristic {
public:
    HeuristicResult operator()(const SylvanZddCnf &cnf) {
        SylvanZddCnf::Literal l = cnf.get_root_literal();
        LOG_INFO << "Heuristic found root literal " << l;
        return {true, l, 0};
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

enum class ScoreEvaluator {
    Bloat,
};

template<ScoreEvaluator Evaluator = ScoreEvaluator::Bloat>
class MinimalScoreHeuristic {
public:
    MinimalScoreHeuristic(size_t min_var, size_t max_var) : m_min_var(min_var), m_max_var(max_var) {}

    HeuristicResult operator()(const SylvanZddCnf &cnf) {
        SylvanZddCnf::FormulaStats stats = cnf.get_formula_statistics();
        // minimize score
        Score best_score = std::numeric_limits<Score>::max();
        size_t best_var = 0;
        const size_t min_var = std::max(m_min_var, stats.index_shift);
        const size_t max_var = std::min(m_max_var, stats.vars.size() - stats.index_shift);
        for (size_t var = min_var; var <= max_var; ++var) {
            size_t idx = var - stats.index_shift;
            const SylvanZddCnf::VariableStats &var_stats = stats.vars[idx];
            // skip missing variables
            if (var_stats.positive_clause_count == 0 && var_stats.negative_clause_count == 0) {
                continue;
            }
            Score score = calculate_score(var_stats);
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
    using Score = int64_t;

    const size_t m_min_var;
    const size_t m_max_var;

    static Score calculate_score(const SylvanZddCnf::VariableStats &stats) {
        if constexpr (Evaluator == ScoreEvaluator::Bloat) {
            return bloat_score(stats);
        }
    }

    static Score bloat_score(const SylvanZddCnf::VariableStats &stats) {
        auto removed_clauses = static_cast<Score>(stats.positive_clause_count + stats.negative_clause_count);
        auto new_clauses = static_cast<Score>(stats.positive_clause_count * stats.negative_clause_count);
        return new_clauses - removed_clauses;
    }
};

} // namespace dp
