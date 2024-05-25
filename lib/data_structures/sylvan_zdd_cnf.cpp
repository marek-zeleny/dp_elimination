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
    LaceActivator lace;
    sylvan_table_usage(&stats.table_filled, &stats.table_total);
    return stats;
}

void SylvanZddCnf::call_sylvan_gc() {
    LOG_DEBUG << "Calling Sylvan GC manually";
    LaceActivator lace;
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
        sylvan_gc_hook_pregc(TASK(sylvan_log_before_gc));
        sylvan_gc_hook_postgc(TASK(sylvan_log_after_gc));
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
    LaceActivator lace;
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

    LaceActivator lace;
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
    LaceActivator lace;
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
    LaceActivator lace;
    ZDD zdd = zdd_eval(m_zdd, v, 0);
    return SylvanZddCnf(zdd);
}

SylvanZddCnf SylvanZddCnf::subset1(Literal l) const {
    Var v = literal_to_var(l);
    LaceActivator lace;
    ZDD zdd = zdd_eval(m_zdd, v, 1);
    return SylvanZddCnf(zdd);
}

SylvanZddCnf SylvanZddCnf::unify(const SylvanZddCnf &other) const {
    assert(verify_variable_ordering());
    assert(other.verify_variable_ordering());
    LaceActivator lace;
    ZDD zdd = zdd_or(m_zdd, other.m_zdd);
    return SylvanZddCnf(zdd);
}

SylvanZddCnf SylvanZddCnf::unify_and_remove_subsumed(const SylvanZddCnf &other) const {
    assert(verify_variable_ordering());
    assert(other.verify_variable_ordering());
    LaceActivator lace;
    ZDD zdd = zdd_or_no_subsumed(m_zdd, other.m_zdd);
    return SylvanZddCnf(zdd);
}

SylvanZddCnf SylvanZddCnf::intersect(const SylvanZddCnf &other) const {
    LaceActivator lace;
    ZDD zdd = zdd_and(m_zdd, other.m_zdd);
    return SylvanZddCnf(zdd);
}

SylvanZddCnf SylvanZddCnf::subtract(const SylvanZddCnf &other) const {
    LaceActivator lace;
    ZDD zdd = zdd_diff(m_zdd, other.m_zdd);
    return SylvanZddCnf(zdd);
}

SylvanZddCnf SylvanZddCnf::subtract_subsumed(const dp::SylvanZddCnf &other) const {
    LaceActivator lace;
    ZDD zdd = zdd_subsumed_diff(m_zdd, other.m_zdd);
    return SylvanZddCnf(zdd);
}

SylvanZddCnf SylvanZddCnf::multiply(const SylvanZddCnf &other) const {
    LaceActivator lace;
    ZDD zdd = zdd_product(m_zdd, other.m_zdd);
    return SylvanZddCnf(zdd);
}

SylvanZddCnf SylvanZddCnf::remove_tautologies() const {
    LaceActivator lace;
    ZDD zdd = zdd_remove_tautologies(m_zdd);
    return SylvanZddCnf(zdd);
}

SylvanZddCnf SylvanZddCnf::remove_subsumed_clauses() const {
    LaceActivator lace;
    ZDD zdd = zdd_no_subsumed(m_zdd);
    return SylvanZddCnf(zdd);
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

SylvanZddCnf::LaceActivator::LaceActivator() {
    if (s_depth++ == 0) {
        lace_resume();
    }
}

SylvanZddCnf::LaceActivator::~LaceActivator() {
    if (--s_depth == 0) {
        lace_suspend();
    }
}

SylvanZddCnf::LogarithmicBuilder::LogarithmicBuilder() : m_forest({{zdd_false, 0}}) {
    zdd_protect(&std::get<0>(m_forest.front()));
}

SylvanZddCnf::LogarithmicBuilder::~LogarithmicBuilder() {
    for (auto &level : m_forest) {
        zdd_unprotect(&std::get<0>(level));
    }
}

void SylvanZddCnf::LogarithmicBuilder::add_clause(const dp::SylvanZddCnf::Clause &c) {
    check_and_merge();

    ZDD clause = clause_from_vector(c);
    zdd_protect(&clause);
    auto &top = m_forest.front();
    ZDD &zdd = std::get<0>(top);
    size_t &size = std::get<1>(top);
    assert(verify_variable_ordering_impl(clause, 0));
    assert(verify_variable_ordering_impl(zdd, 0));
    LaceActivator lace;
    zdd = zdd_or(zdd, clause);
    zdd_unprotect(&clause);
    ++size;
}

ZDD SylvanZddCnf::LogarithmicBuilder::get_result() const {
    LOG_DEBUG << "Getting result from logarithmic ZDD builder, unifying " << m_forest.size() << " trees at "
              << std::bit_width(std::get<1>(m_forest.back()) / unit_size) << " levels";
    ZDD result = zdd_false;
    zdd_protect(&result);
    LaceActivator lace;
    for (const auto &level : m_forest) {
        result = zdd_or(result, std::get<0>(level));
    }
    zdd_unprotect(&result);
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
    LaceActivator lace;
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
        zdd_unprotect(&std::get<0>(m_forest.front()));
        m_forest.pop_front();
        top_zdd = prev_zdd;
        top_size = prev_size;
    }
    m_forest.emplace_front(zdd_false, 0);
    zdd_protect(&std::get<0>(m_forest.front()));
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

ZDD SylvanZddCnf::clause_from_vector(const Clause &clause) {
    std::vector<Var> vars;
    vars.reserve(clause.size());
    for (auto &l : clause) {
        Var v = literal_to_var(l);
        vars.push_back(v);
    }
    std::sort(vars.begin(), vars.end());
    return zdd_combination_from_array(vars.data(), vars.size());
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