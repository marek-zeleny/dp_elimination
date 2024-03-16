#include "data_structures/sylvan_zdd_cnf.hpp"

#include <algorithm>
#include <tuple>
#include <stack>
#include <unordered_set>
#include <unordered_map>
#include <iostream>
#include <cstdlib>
#include <cstdio>
#include <cerrno>
#include <cstring>
#include <cassert>
#include <sylvan.h>
#include <simple_logger.h>

#include "io/cnf_reader.hpp"
#include "io/cnf_writer.hpp"

namespace dp {

SylvanZddCnf::SylvanZddCnf() : m_zdd(zdd_false) {
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

SylvanZddCnf SylvanZddCnf::from_vector(const std::vector<Clause> &clauses) {
    ZDD zdd = zdd_false;
    ZDD clause = zdd_false;
    zdd_refs_pushptr(&zdd);
    zdd_refs_pushptr(&clause);
    for (auto &c: clauses) {
        clause = clause_from_vector(c);
        zdd = zdd_or(zdd, clause);
    }
    zdd_refs_popptr(2);
    return SylvanZddCnf(zdd);
}

SylvanZddCnf SylvanZddCnf::from_file(const std::string &file_name) {
    ZDD zdd = zdd_false;
    ZDD clause = zdd_false;
    zdd_refs_pushptr(&zdd);
    zdd_refs_pushptr(&clause);
    CnfReader::AddClauseFunction func = [&](const CnfReader::Clause &c) {
        clause = clause_from_vector(c);
        zdd = zdd_or(zdd, clause);
    };
    try {
        CnfReader::read_from_file(file_name, func);
    } catch (const CnfReader::failure &f) {
        std::cerr << f.what() << std::endl;
        zdd_refs_popptr(2);
        return {};
    }
    zdd_refs_popptr(2);
    return SylvanZddCnf(zdd);
}

size_t SylvanZddCnf::clauses_count() const {
    return zdd_satcount(m_zdd);
}

bool SylvanZddCnf::is_empty() const {
    return m_zdd == zdd_false;
}

bool SylvanZddCnf::contains_empty() const {
    return contains_empty_set(m_zdd);
}

SylvanZddCnf::Literal SylvanZddCnf::get_smallest_variable() const {
    if (m_zdd == zdd_false || m_zdd == zdd_true) {
        return 0;
    }
    Var v = zdd_getvar(m_zdd);
    return std::abs(var_to_literal(v));
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
    ZDD result;
    std::tuple<ZDD, ZDD> cache_key = std::make_tuple(p, q);
    if (s_multiply_cache.try_get(cache_key, result)) {
        return result;
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
    ZDD tmp = zdd_or(zdd_or(p1q1, p1q0), p0q1);
    zdd_refs_push(tmp);
    result = zdd_makenode(x, p0q0, tmp);
    zdd_refs_pop(5);
    s_multiply_cache.add(cache_key, result);
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
    ZDD result;
    if (s_remove_tautologies_cache.try_get(zdd, result)) {
        return result;
    }
    // recursive step
    Var var = zdd_getvar(zdd);
    ZDD low = remove_tautologies_impl(zdd_getlow(zdd));
    zdd_refs_push(low);
    ZDD high = remove_tautologies_impl(zdd_gethigh(zdd));
    zdd_refs_push(high);
    Var high_var = zdd_getvar(high);
    if (high == zdd_false || high == zdd_true) {
        // high child doesn't have a variable (is a leaf), do nothing
        result = zdd_makenode(var, low, high);
    } else if (var / 2 == high_var / 2) {
        // otherwise compare variables in <zdd> and <high>
        // complements, remove high child of <high>
        result = zdd_makenode(var, low, zdd_getlow(high));
    } else {
        // not complements, do nothing
        result = zdd_makenode(var, low, high);
    }
    zdd_refs_pop(2);
    s_remove_tautologies_cache.add(zdd, result);
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
    ZDD result;
    if (s_remove_subsumed_clauses_cache.try_get(zdd, result)) {
        return result;
    }
    // recursive step
    Var var = zdd_getvar(zdd);
    ZDD low = remove_subsumed_clauses_impl(zdd_getlow(zdd));
    zdd_refs_push(low);
    ZDD high = remove_subsumed_clauses_impl(zdd_gethigh(zdd));
    zdd_refs_push(high);
    ZDD high_without_supersets = remove_supersets(high, low);
    zdd_refs_push(high_without_supersets);
    result = zdd_makenode(var, low, high_without_supersets);
    zdd_refs_pop(3);
    s_remove_subsumed_clauses_cache.add(zdd, result);
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
    ZDD result;
    std::tuple<ZDD, ZDD> cache_key = std::make_tuple(p, q);
    if (s_remove_supersets_cache.try_get(cache_key, result)) {
        return result;
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
    result = zdd_makenode(top_var, low, high);
    zdd_refs_pop(4);
    s_remove_supersets_cache.add(cache_key, result);
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
    return output;
}

void SylvanZddCnf::print_clauses() const {
    print_clauses(std::cout);
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

bool SylvanZddCnf::draw_to_file(std::FILE *file) const {
    // Not ideal error handling, but zdd_fprintdot() returns void...
    int prev_errno = errno;
    zdd_fprintdot(file, m_zdd);
    if (errno != prev_errno) {
        std::cerr << "Error while drawing sylvan ZDD to file: " << std::strerror(errno) << std::endl;
        return false;
    }
    return true;
}

bool SylvanZddCnf::draw_to_file(const std::string &file_name) const {
    std::FILE *file = std::fopen(file_name.c_str(), "w");
    if (file == nullptr) {
        std::cerr << "Error while drawing sylvan ZDD to file: failed to open the output file" << std::endl;
        return false;
    }
    bool retval = draw_to_file(file);
    if (std::fclose(file) != 0) {
        std::cerr << "Error while drawing sylvan ZDD to file: failed to close the output file" << std::endl;
        return false;
    }
    return retval;
}

bool SylvanZddCnf::write_dimacs_to_file(const std::string &file_name) const {
    auto max_var = static_cast<size_t>(get_largest_variable());
    size_t num_clauses = clauses_count();
    try {
        CnfWriter writer(file_name, max_var, num_clauses);
        ClauseFunction func = [&](const Clause &clause) {
            writer.write_clause(clause);
            return true;
        };
        for_all_clauses(func);
        writer.finish();
    } catch (const CnfWriter::failure &f) {
        std::cerr << f.what();
        return false;
    }
    return true;
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
    ZDD zdd_clause = zdd_cube(domain, signs.data(), zdd_true);
    return zdd_clause;
}

bool SylvanZddCnf::contains_empty_set(const ZDD &zdd) {
    return (zdd & zdd_complement) != 0;
}

} // namespace dp