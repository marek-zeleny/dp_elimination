#include "data_structures/sylvan_zdd_cnf.hpp"

#include <algorithm>
#include <tuple>
#include <stack>
#include <unordered_set>
#include <unordered_map>
#include <bit>
#include <stdexcept>
#include <cstdlib>
#include <cstdio>
#include <cerrno>
#include <cstring>
#include <cassert>
#include <sylvan.h>
#include <simple_logger.h>

#include "io/cnf_reader.hpp"
#include "io/cnf_writer.hpp"
#include "metrics/dp_metrics.hpp"

using namespace sylvan;

namespace dp {

SylvanZddCnf::SylvanStats SylvanZddCnf::get_sylvan_stats() {
    SylvanStats stats{};
    sylvan_table_usage(&stats.table_filled, &stats.table_total);
    return stats;
}

void SylvanZddCnf::call_sylvan_gc() {
    LOG_DEBUG << "Calling Sylvan GC manually";
    sylvan_clear_cache();
    sylvan_clear_and_mark();
    sylvan_rehash_all();
}

namespace {

VOID_TASK_0(sylvan_log_before_gc) {
    auto stats = SylvanZddCnf::get_sylvan_stats();
    LOG_DEBUG << "Sylvan: calling GC, table usage " << stats.table_filled << "/" << stats.table_total;
}

VOID_TASK_0(sylvan_log_after_gc) {
    auto stats = SylvanZddCnf::get_sylvan_stats();
    LOG_DEBUG << "Sylvan: GC complete, table usage " << stats.table_filled << "/" << stats.table_total;
}

} // namespace

void SylvanZddCnf::hook_sylvan_gc_log() {
    if constexpr (simple_logger::Log<simple_logger::LogLevel::Debug>::isActive) {
        sylvan_gc_hook_pregc(sylvan_log_before_gc_CALL);
        sylvan_gc_hook_postgc(sylvan_log_after_gc_CALL);
    }
}

SylvanZddCnf::SylvanZddCnf() : m_zdd(zdd_false) {
    static_assert(std::is_same_v<SylvanZddCnf::ZDD, sylvan::ZDD>);
    zdd_protect(&m_zdd);
}

SylvanZddCnf::SylvanZddCnf(ZDD zdd) : m_zdd(zdd) {
    zdd_protect(&m_zdd);
}

SylvanZddCnf::SylvanZddCnf(const SylvanZddCnf &other) : m_zdd(other.m_zdd) {
    zdd_protect(&m_zdd);
}

SylvanZddCnf &SylvanZddCnf::operator=(const SylvanZddCnf &other) {
    m_zdd = other.m_zdd;
    return *this;
}

SylvanZddCnf::~SylvanZddCnf() {
    zdd_unprotect(&m_zdd);
}

static bool verify_variable_ordering_impl(const ZDD &node, uint32_t parent_var) {
    if (node == zdd_true || node == zdd_false) {
        return true;
    }
    uint32_t var = zdd_getvar(node);
    if (var <= parent_var) {
        LOG_ERROR << "Invalid ZDD ordering found: node " << node << ", var " << var << ", parent " << parent_var;
        return false;
    }
    if (!verify_variable_ordering_impl(zdd_getlow(node), var)) {
        return false;
    }
    return verify_variable_ordering_impl(zdd_gethigh(node), var);
}

bool SylvanZddCnf::verify_variable_ordering() const {
    return verify_variable_ordering_impl(m_zdd, 0);
}

SylvanZddCnf SylvanZddCnf::from_vector(const std::vector<Clause> &clauses) {
    LogarithmicBuilder builder;
    for (const auto &c : clauses) {
        builder.add_clause(c);
    }
    ZDD zdd = builder.get_result();
    return SylvanZddCnf(zdd);
}

SylvanZddCnf SylvanZddCnf::from_file(const std::string &file_name) {
    auto timer = metrics.get_timer(MetricsDurations::ReadInputFormula);
    size_t clause_count = 0;
    LogarithmicBuilder builder;
    CnfReader::AddClauseFunction func = [&builder, &clause_count](const CnfReader::Clause &c) {
        ++clause_count;
        auto timer = metrics.get_timer(MetricsDurations::ReadFormula_AddClause);
        builder.add_clause(c);
    };
    try {
        CnfReader::read_from_file(file_name, func);
    } catch (const CnfReader::failure &f) {
        LOG_ERROR << f.what();
        throw;
    }
    ZDD zdd = builder.get_result();
    assert(builder.get_size() == clause_count);
    assert(zdd_satcount(zdd) == clause_count);
    return SylvanZddCnf(zdd);
}

size_t SylvanZddCnf::count_clauses() const {
    return zdd_satcount(m_zdd);
}

size_t SylvanZddCnf::count_nodes() const {
    return zdd_nodecount_one(m_zdd);
}

static size_t count_depth_impl(const ZDD &zdd) {
    if (zdd == zdd_true || zdd == zdd_false) {
        return 0;
    } else {
        return std::max(count_depth_impl(zdd_getlow(zdd)), count_depth_impl(zdd_gethigh(zdd))) + 1;
    }
}

size_t SylvanZddCnf::count_depth() const {
    return count_depth_impl(m_zdd);
}

bool SylvanZddCnf::is_empty() const {
    return m_zdd == zdd_false;
}

bool SylvanZddCnf::contains_empty() const {
    return contains_empty_set(m_zdd);
}

bool SylvanZddCnf::contains_unit_literal(const Literal &literal) const {
    Var searched_var = literal_to_var(literal);
    ZDD zdd = m_zdd;
    Var v = zdd_getvar(zdd);
    while (v < searched_var && zdd != zdd_false && zdd != zdd_true) {
        zdd = zdd_getlow(zdd);
        v = zdd_getvar(zdd);
    }
    return v == searched_var && contains_empty_set(zdd_gethigh(zdd));
}

SylvanZddCnf::Literal SylvanZddCnf::get_smallest_variable() const {
    return std::abs(get_root_literal());
}

namespace {

uint32_t get_largest_variable_impl(const ZDD &zdd) {
    if (zdd == zdd_true || zdd == zdd_false) {
        return 0;
    }
    uint32_t var = zdd_getvar(zdd);
    uint32_t low = get_largest_variable_impl(zdd_getlow(zdd));
    uint32_t high = get_largest_variable_impl(zdd_gethigh(zdd));
    return std::max({var, low, high});
}

} // namespace

SylvanZddCnf::Literal SylvanZddCnf::get_largest_variable() const {
    if (m_zdd == zdd_true || m_zdd == zdd_false) {
        return 0;
    }
    Var v = get_largest_variable_impl(m_zdd);
    return std::abs(var_to_literal(v));
}

SylvanZddCnf::Literal SylvanZddCnf::get_root_literal() const {
    if (m_zdd == zdd_true || m_zdd == zdd_false) {
        return 0;
    }
    Var v = zdd_getvar(m_zdd);
    return var_to_literal(v);
}

SylvanZddCnf::Literal SylvanZddCnf::get_unit_literal() const {
    ZDD zdd = m_zdd;
    while (zdd != zdd_false && zdd != zdd_true) {
        // if high child contains empty set, current node contains a unit literal
        ZDD high = zdd_gethigh(zdd);
        if (contains_empty_set(high)) {
            return var_to_literal(zdd_getvar(zdd));
        } else {
            // otherwise, follow the low path
            zdd = zdd_getlow(zdd);
        }
    }
    // if no unit literal was found, return invalid literal
    return 0;
}

SylvanZddCnf::Literal SylvanZddCnf::get_clear_literal() const {
    using Occurrence = uint8_t;
    constexpr Occurrence POSITIVE = 1 << 0;
    constexpr Occurrence NEGATIVE = 1 << 1;
    std::stack<ZDD> stack;
    stack.push(m_zdd);
    std::unordered_set<ZDD> visited;
    std::unordered_map<Literal, Occurrence> occurrences;
    while (!stack.empty()) {
        ZDD zdd = stack.top();
        stack.pop();
        if (zdd == zdd_false || zdd == zdd_true || visited.contains(zdd)) {
            continue;
        }
        visited.insert(zdd);
        stack.push(zdd_getlow(zdd));
        stack.push(zdd_gethigh(zdd));
        Literal l = var_to_literal(zdd_getvar(zdd));
        Occurrence occ = l > 0 ? POSITIVE : NEGATIVE;
        Literal var = std::abs(l);
        if (occurrences.contains(var)) {
            occurrences[var] |= occ;
        } else {
            occurrences[var] = occ;
        }
    }
    for (auto &[var, occ] : occurrences) {
        if (occ != (POSITIVE | NEGATIVE)) {
            if (occ == POSITIVE) {
                return var;
            } else {
                assert(occ == NEGATIVE);
                return -var;
            }
        }
    }
    return 0;
}

SylvanZddCnf::FormulaStats SylvanZddCnf::get_formula_statistics() const {
    FormulaStats stats;
    stats.index_shift = get_smallest_variable();
    stats.vars.resize(get_largest_variable() - stats.index_shift + 1);
    ClauseFunction func = [&](const Clause &clause) {
        for (auto &l : clause) {
            size_t idx = std::abs(l) - stats.index_shift;
            VariableStats &var_stats = stats.vars[idx];
            if (l > 0) {
                ++var_stats.positive_clause_count;
            } else {
                ++var_stats.negative_clause_count;
            }
        }
        return true;
    };
    for_all_clauses(func);
    return stats;
}

SylvanZddCnf SylvanZddCnf::subset0(Literal l) const {
    Var v = literal_to_var(l);
    ZDD zdd = zdd_eval(m_zdd, v, 0);
    return SylvanZddCnf(zdd);
}

SylvanZddCnf SylvanZddCnf::subset1(Literal l) const {
    Var v = literal_to_var(l);
    ZDD zdd = zdd_eval(m_zdd, v, 1);
    return SylvanZddCnf(zdd);
}

SylvanZddCnf SylvanZddCnf::unify(const SylvanZddCnf &other) const {
    assert(verify_variable_ordering());
    assert(other.verify_variable_ordering());
    ZDD zdd = zdd_or(m_zdd, other.m_zdd);
    return SylvanZddCnf(zdd);
}

SylvanZddCnf SylvanZddCnf::intersect(const SylvanZddCnf &other) const {
    ZDD zdd = zdd_and(m_zdd, other.m_zdd);
    return SylvanZddCnf(zdd);
}

SylvanZddCnf SylvanZddCnf::subtract(const SylvanZddCnf &other) const {
    ZDD zdd = zdd_diff(m_zdd, other.m_zdd);
    return SylvanZddCnf(zdd);
}

SylvanZddCnf SylvanZddCnf::multiply(const SylvanZddCnf &other) const {
    ZDD zdd = multiply_impl(m_zdd, other.m_zdd);
    return SylvanZddCnf(zdd);
}

ZDD SylvanZddCnf::multiply_impl(const ZDD &p, const ZDD &q) {
    // TODO: maybe later implement this as a Lace task and compute it in parallel
    // resolve ground cases
    if (p == zdd_false) {
        return zdd_false;
    }
    if (p == zdd_true) {
        return q;
    }
    if (q == zdd_false) {
        return zdd_false;
    }
    if (q == zdd_true) {
        return p;
    }
    // break symmetry
    Var p_var = zdd_getvar(p);
    Var q_var = zdd_getvar(q);
    if (p_var > q_var) {
        return multiply_impl(q, p);
    }
    // look into the cache
    std::optional<ZDD> cache_result = try_get_from_binary_cache(s_multiply_cache, p, q);
    if (cache_result.has_value()) {
        return *cache_result;
    }
    // general case (recursion)
    Var x = p_var;
    ZDD p0 = zdd_getlow(p);
    ZDD p1 = zdd_gethigh(p);
    ZDD q0;
    ZDD q1;
    if (q_var == p_var) {
        q0 = zdd_getlow(q);
        q1 = zdd_gethigh(q);
    } else {
        q0 = q;
        q1 = zdd_false;
    }
    // NOTE: p0, p1, q0, q1 are protected from GC recursively -> no need to push references
    ZDD p0q0 = multiply_impl(p0, q0);
    zdd_refs_push(p0q0);
    ZDD p0q1 = multiply_impl(p0, q1);
    zdd_refs_push(p0q1);
    ZDD p1q0 = multiply_impl(p1, q0);
    zdd_refs_push(p1q0);
    ZDD p1q1 = multiply_impl(p1, q1);
    zdd_refs_push(p1q1);
    assert(verify_variable_ordering_impl(p1q1, 0));
    assert(verify_variable_ordering_impl(p1q0, 0));
    ZDD tmp1 = zdd_or(p1q1, p1q0);
    zdd_refs_pop(2);
    zdd_refs_push(tmp1);
    assert(verify_variable_ordering_impl(tmp1, 0));
    assert(verify_variable_ordering_impl(p0q1, 0));
    ZDD tmp2 = zdd_or(tmp1, p0q1);
    zdd_refs_pop(3);
    ZDD result = zdd_makenode(x, p0q0, tmp2);
    store_in_binary_cache(s_multiply_cache, p, q, result);
    return result;
}

SylvanZddCnf SylvanZddCnf::remove_tautologies() const {
    ZDD zdd = remove_tautologies_impl(m_zdd);
    return SylvanZddCnf(zdd);
}

ZDD SylvanZddCnf::remove_tautologies_impl(const ZDD &zdd) {
    // NOTE: this algorithm assumes that complementary literals are consecutive in the variable order,
    //       i.e. for x > 0: var(x) = 2x, var(-x) = 2x + 1
    // TODO: consider caching and/or parallel implementation using Lace
    // resolve ground cases
    if (zdd == zdd_false || zdd == zdd_true) {
        return zdd;
    }
    // look into the cache
    std::optional<ZDD> cache_result = try_get_from_unary_cache(s_remove_tautologies_cache, zdd);
    if (cache_result.has_value()) {
        return *cache_result;
    }
    // recursive step
    Var var = zdd_getvar(zdd);
    ZDD low = remove_tautologies_impl(zdd_getlow(zdd));
    zdd_refs_push(low);
    ZDD high = remove_tautologies_impl(zdd_gethigh(zdd));
    zdd_refs_pop(1);
    ZDD result;
    if (high == zdd_false || high == zdd_true) {
        // high child doesn't have a variable (is a leaf), do nothing
        result = zdd_makenode(var, low, high);
    } else if (var / 2 == zdd_getvar(high) / 2) {
        // otherwise compare variables in <zdd> and <high>
        // complements, remove high child of <high>
        result = zdd_makenode(var, low, zdd_getlow(high));
    } else {
        // not complements, do nothing
        result = zdd_makenode(var, low, high);
    }
    store_in_unary_cache(s_remove_tautologies_cache, zdd, result);
    return result;
}

SylvanZddCnf SylvanZddCnf::remove_subsumed_clauses() const {
    ZDD zdd = remove_subsumed_clauses_impl(m_zdd);
    return SylvanZddCnf(zdd);
}

ZDD SylvanZddCnf::remove_subsumed_clauses_impl(const ZDD &zdd) {
    // resolve ground cases
    if (zdd == zdd_false || zdd == zdd_true) {
        return zdd;
    }
    // look into the cache
    std::optional<ZDD> cache_result = try_get_from_unary_cache(s_remove_subsumed_clauses_cache, zdd);
    if (cache_result.has_value()) {
        return *cache_result;
    }
    // recursive step
    Var var = zdd_getvar(zdd);
    ZDD low = remove_subsumed_clauses_impl(zdd_getlow(zdd));
    zdd_refs_push(low);
    ZDD high = remove_subsumed_clauses_impl(zdd_gethigh(zdd));
    zdd_refs_push(high);
    ZDD high_without_supersets = remove_supersets(high, low);
    zdd_refs_push(high_without_supersets);
    ZDD result = zdd_makenode(var, low, high_without_supersets);
    zdd_refs_pop(3);
    store_in_unary_cache(s_remove_subsumed_clauses_cache, zdd, result);
    return result;
}

ZDD SylvanZddCnf::remove_supersets(const ZDD &p, const ZDD &q) {
    // resolve ground cases
    if (p == zdd_false || contains_empty_set(q) || p == q) {
        return zdd_false;
    }
    if (p == zdd_true || q == zdd_false) {
        return p;
    }
    // look into the cache
    std::optional<ZDD> cache_result = try_get_from_binary_cache(s_remove_supersets_cache, p, q);
    if (cache_result.has_value()) {
        return *cache_result;
    }
    // recursive step
    Var p_var = zdd_getvar(p);
    Var q_var = zdd_getvar(q);
    Var top_var = std::min(p_var, q_var);
    ZDD p0 = zdd_eval(p, top_var, 0);
    ZDD p1 = zdd_eval(p, top_var, 1);
    ZDD q0 = zdd_eval(q, top_var, 0);
    ZDD q1 = zdd_eval(q, top_var, 1);
    // NOTE: p0, p1, q0, q1 are protected from GC recursively -> no need to push references
    ZDD tmp1 = remove_supersets(p1, q0);
    zdd_refs_push(tmp1);
    ZDD tmp2 = remove_supersets(p1, q1);
    zdd_refs_push(tmp2);
    ZDD low = remove_supersets(p0, q0);
    zdd_refs_push(low);
    ZDD high = zdd_and(tmp1, tmp2);
    zdd_refs_push(high);
    ZDD result = zdd_makenode(top_var, low, high);
    zdd_refs_pop(4);
    store_in_binary_cache(s_remove_supersets_cache, p, q, result);
    return result;
}

void SylvanZddCnf::for_all_clauses(ClauseFunction &func) const {
    Clause stack;
    for_all_clauses_impl(func, m_zdd, stack);
}

bool SylvanZddCnf::for_all_clauses_impl(ClauseFunction &func, const ZDD &node, Clause &stack) {
    if (node == zdd_true) {
        return func(stack);
    } else if (node == zdd_false) {
        return true;
    }
    if (!for_all_clauses_impl(func, zdd_getlow(node), stack)) {
        return false;
    }
    Literal l = var_to_literal(zdd_getvar(node));
    stack.push_back(l);
    if (!for_all_clauses_impl(func, zdd_gethigh(node), stack)) {
        return false;
    }
    stack.pop_back();
    return true;
}

auto SylvanZddCnf::to_vector() const -> std::vector<Clause> {
    std::vector<Clause> output;
    ClauseFunction func = [&](const Clause &clause) {
        output.emplace_back(clause);
        return true;
    };
    for_all_clauses(func);
    assert(output.size() == count_clauses());
    return output;
}

void SylvanZddCnf::print_clauses(std::ostream &output) const {
    ClauseFunction func = [&](const Clause &clause) {
        output << "{";
        for (auto &l: clause) {
            output << " " << l << ",";
        }
        output << "}\n";
        return true;
    };
    for_all_clauses(func);
}

void SylvanZddCnf::draw_to_file(std::FILE *file) const {
    // Not ideal error handling, but zdd_fprintdot() returns void...
    int prev_errno = errno;
    zdd_fprintdot(file, m_zdd);
    if (errno != prev_errno) {
        std::string error_msg{std::string("Error while drawing sylvan ZDD to file: ") + std::strerror(errno)};
        LOG_ERROR << error_msg;
        throw std::runtime_error(error_msg);
    }
}

void SylvanZddCnf::draw_to_file(const std::string &file_name) const {
    std::FILE *file = std::fopen(file_name.c_str(), "w");
    if (file == nullptr) {
        std::string error_msg{"Error while drawing sylvan ZDD to file: failed to open the output file"};
        LOG_ERROR << error_msg;
        throw std::runtime_error(error_msg);
    }
    draw_to_file(file);
    if (std::fclose(file) != 0) {
        std::string error_msg{"Error while drawing sylvan ZDD to file: failed to close the output file"};
        LOG_ERROR << error_msg;
        throw std::runtime_error(error_msg);
    }
}

void SylvanZddCnf::write_dimacs_to_file(const std::string &file_name) const {
    auto timer = metrics.get_timer(MetricsDurations::WriteOutputFormula);
    auto max_var = static_cast<size_t>(get_largest_variable());
    size_t num_clauses = count_clauses();
    try {
        CnfWriter writer(file_name, max_var, num_clauses);
        ClauseFunction func = [&writer](const Clause &clause) {
            auto timer = metrics.get_timer(MetricsDurations::WriteFormula_PrintClause);
            writer.write_clause(clause);
            return true;
        };
        for_all_clauses(func);
        writer.finish();
    } catch (const CnfWriter::failure &f) {
        LOG_ERROR << f.what();
        throw;
    }
}

SylvanZddCnf::LogarithmicBuilder::LogarithmicBuilder() : m_forest({{zdd_false, 0}}) {
    zdd_refs_pushptr(&std::get<0>(m_forest.front()));
}

SylvanZddCnf::LogarithmicBuilder::~LogarithmicBuilder() {
    zdd_refs_popptr(m_forest.size());
}

void SylvanZddCnf::LogarithmicBuilder::add_clause(const dp::SylvanZddCnf::Clause &c) {
    check_and_merge();

    ZDD clause = clause_from_vector(c);
    zdd_refs_push(clause);
    auto &top = m_forest.front();
    ZDD &zdd = std::get<0>(top);
    size_t &size = std::get<1>(top);
    assert(verify_variable_ordering_impl(clause, 0));
    assert(verify_variable_ordering_impl(zdd, 0));
    zdd = zdd_or(zdd, clause);
    zdd_refs_pop(1);
    ++size;
}

ZDD SylvanZddCnf::LogarithmicBuilder::get_result() const {
    LOG_DEBUG << "Getting result from logarithmic ZDD builder, unifying " << m_forest.size() << " trees at "
              << std::bit_width(std::get<1>(m_forest.back()) / unit_size) << " levels";
    ZDD result = zdd_false;
    for (const auto &level : m_forest) {
        result = zdd_or(result, std::get<0>(level));
    }
    return result;
}

size_t SylvanZddCnf::LogarithmicBuilder::get_size() const {
    size_t result = 0;
    for (const auto &level : m_forest) {
        result += std::get<1>(level);
    }
    return result;
}

void SylvanZddCnf::LogarithmicBuilder::check_and_merge() {
    auto [top_zdd, top_size] = m_forest.front();
    if (top_size < unit_size) {
        // top level not saturated, nothing to merge
        return;
    }
    assert(top_size == unit_size);
    while (m_forest.size() > 1) {
        auto &prev = m_forest[1];
        size_t &prev_size = std::get<1>(prev);
        if (prev_size > top_size) {
            // previous tree is on a lower level, shift and insert a new top level
            break;
        }
        // top tree and previous tree are neighbouring levels, merge them and continue loop
        assert(prev_size == top_size);
        ZDD &prev_zdd = std::get<0>(prev);
        prev_zdd = zdd_or(prev_zdd, top_zdd);
        prev_size += top_size;
        zdd_refs_popptr(1);
        m_forest.pop_front();
        top_zdd = prev_zdd;
        top_size = prev_size;
    }
    m_forest.emplace_front(zdd_false, 0);
    zdd_refs_pushptr(&std::get<0>(m_forest.front()));
}

SylvanZddCnf::Var SylvanZddCnf::literal_to_var(Literal l) {
    assert (l != 0);
    if (l > 0) {
        return 2 * static_cast<Var>(l);
    } else {
        return 2 * static_cast<Var>(-l) + 1;
    }
}

SylvanZddCnf::Literal SylvanZddCnf::var_to_literal(Var v) {
    auto [q, r] = std::div(static_cast<Literal>(v), 2);
    if (r == 0) {
        return q;
    } else {
        return -q;
    }
}

ZDD SylvanZddCnf::set_from_vector(const Clause &clause) {
    std::vector<Var> vars;
    for (auto &l: clause) {
        Var v = literal_to_var(l);
        vars.push_back(v);
    }
    std::sort(vars.begin(), vars.end());
    ZDD set = zdd_set_from_array(vars.data(), vars.size());
    return set;
}

ZDD SylvanZddCnf::clause_from_vector(const Clause &clause) {
    std::vector<uint8_t> signs(clause.size(), 1);
    ZDD domain = set_from_vector(clause);
    zdd_refs_push(domain);
    ZDD zdd_clause = zdd_cube(domain, signs.data(), zdd_true);
    zdd_refs_pop(1);
    return zdd_clause;
}

bool SylvanZddCnf::contains_empty_set(const ZDD &zdd) {
    return (zdd & zdd_complement) != 0;
}

void SylvanZddCnf::store_in_unary_cache(UnaryCache &cache, const ZDD &key, const ZDD &entry) {
    if constexpr (UnaryCache::Capacity == 0) {
        return;
    }
    Zdd_ptr key_ptr = std::make_shared<ZDD>(key);
    Zdd_ptr entry_ptr = std::make_shared<ZDD>(entry);
    zdd_protect(key_ptr.get());
    zdd_protect(entry_ptr.get());
    auto removed = cache.add(key_ptr, entry_ptr);
    if (removed.has_value()) {
        zdd_unprotect(std::get<0>(*removed).get());
        zdd_unprotect(std::get<1>(*removed).get());
    }
}

void SylvanZddCnf::store_in_binary_cache(BinaryCache &cache, const ZDD &key1, const ZDD &key2, const ZDD &entry) {
    if constexpr (BinaryCache::Capacity == 0) {
        return;
    }
    Zdd_ptr key1_ptr = std::make_shared<ZDD>(key1);
    Zdd_ptr key2_ptr = std::make_shared<ZDD>(key2);
    Zdd_ptr entry_ptr = std::make_shared<ZDD>(entry);
    zdd_protect(key1_ptr.get());
    zdd_protect(key2_ptr.get());
    zdd_protect(entry_ptr.get());
    BinaryCacheKey key = std::make_tuple(std::move(key1_ptr), std::move(key2_ptr));
    auto removed = cache.add(key, entry_ptr);
    if (removed.has_value()) {
        BinaryCacheKey &removed_key = std::get<0>(*removed);
        zdd_unprotect(std::get<0>(removed_key).get());
        zdd_unprotect(std::get<1>(removed_key).get());
        zdd_unprotect(std::get<1>(*removed).get());
    }
}

std::optional<ZDD> SylvanZddCnf::try_get_from_unary_cache(UnaryCache &cache, const ZDD &key) {
    if constexpr (UnaryCache::Capacity == 0) {
        return std::nullopt;
    }
    Zdd_ptr key_ptr = std::make_shared<ZDD>(key);
    std::optional<Zdd_ptr> result = cache.try_get(key_ptr);
    if (result.has_value()) {
        return *result.value();
    } else {
        return std::nullopt;
    }
}

std::optional<ZDD> SylvanZddCnf::try_get_from_binary_cache(BinaryCache &cache, const ZDD &key1, const ZDD &key2) {
    if constexpr (BinaryCache::Capacity == 0) {
        return std::nullopt;
    }
    Zdd_ptr key1_ptr = std::make_shared<ZDD>(key1);
    Zdd_ptr key2_ptr = std::make_shared<ZDD>(key2);
    BinaryCacheKey key = std::make_tuple(std::move(key1_ptr), std::move(key2_ptr));
    std::optional<Zdd_ptr> result = cache.try_get(key);
    if (result.has_value()) {
        return *result.value();
    } else {
        return std::nullopt;
    }
}

} // namespace dp