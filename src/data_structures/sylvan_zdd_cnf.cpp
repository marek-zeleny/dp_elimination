#include "data_structures/sylvan_zdd_cnf.hpp"

#include <algorithm>
#include <tuple>
#include <cstdlib>
#include <cstdio>
#include <cassert>
#include <sylvan.h>

#include "utils.hpp"
#include "io/cnf_reader.hpp"
#include "data_structures/lru_cache.hpp"

namespace dp {

SylvanZddCnf::SylvanZddCnf() : m_zdd(zdd_false) {
    zdd_protect(&m_zdd);
}

SylvanZddCnf::SylvanZddCnf(ZDD &zdd) : m_zdd(zdd) {
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

SylvanZddCnf::Heuristic SylvanZddCnf::get_new_heuristic() {
    return Heuristic();
}

SylvanZddCnf SylvanZddCnf::from_vector(const std::vector<Clause> &clauses) {
    ZDD zdd = zdd_false;
    for (auto &&c : clauses) {
        ZDD clause = clause_from_vector(c);
        zdd = zdd_or(zdd, clause);
    }
    return SylvanZddCnf(zdd);
}

SylvanZddCnf SylvanZddCnf::from_file(const std::string &file_name) {
    ZDD zdd = zdd_false;
    CnfReader::AddClauseFunction func = [&](const CnfReader::Clause &c) {
        ZDD clause = clause_from_vector(c);
        zdd = zdd_or(zdd, clause);
    };
    try {
        CnfReader::read_from_file(file_name, func);
    } catch (const CnfReader::warning &w) {
        std::cerr << w.what() << std::endl;
    } catch (const CnfReader::failure &f) {
        std::cerr << f.what() << std::endl;
        return SylvanZddCnf();
    }
    return SylvanZddCnf(zdd);
}

bool SylvanZddCnf::is_empty() const {
    return m_zdd == zdd_false;
}

bool SylvanZddCnf::contains_empty_clause() const {
    return (m_zdd & zdd_complement) != 0;
}

SylvanZddCnf SylvanZddCnf::subset0(Literal l) const {
    // TODO: efficient implementation
    std::vector<Clause> clauses;
    ClauseFunction func = [&](const SylvanZddCnf &, const Clause &clause) {
        auto it = std::find(clause.cbegin(), clause.cend(), l);
        if (it == clause.cend()) {
            clauses.push_back(clause);
        }
        return true;
    };
    for_all_clauses(func);
    return from_vector(clauses);
}

SylvanZddCnf SylvanZddCnf::subset1(Literal l) const {
    // TODO: efficient implementation
    std::vector<Clause> clauses;
    ClauseFunction func = [&](const SylvanZddCnf &, const Clause &clause) {
        auto it = std::find(clause.cbegin(), clause.cend(), l);
        if (it != clause.cend()) {
            Clause without_l;
            for (auto &&x : clause) {
                if (x != l) {
                    without_l.push_back(x);
                }
            }
            clauses.push_back(without_l);
        }
        return true;
    };
    for_all_clauses(func);
    return from_vector(clauses);
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

namespace {

#define MULT_CACHE_SIZE 4

using MultCacheEntry = std::tuple<ZDD, ZDD>;

struct zdd_pair_hash {
    size_t operator()(const MultCacheEntry &pair) const {
        size_t seed = 0;
        hash_combine(seed, std::get<0>(pair));
        hash_combine(seed, std::get<1>(pair));
        return seed;
    }
};

using MultCache = LruCache<MultCacheEntry, ZDD, MULT_CACHE_SIZE, zdd_pair_hash>;

ZDD multiply_impl(const ZDD &p, const ZDD &q, MultCache &cache) {
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
    uint32_t p_var = zdd_getvar(p);
    uint32_t q_var = zdd_getvar(q);
    if (p_var > q_var) {
        return multiply_impl(q, p, cache);
    }
    // look into the cache
    ZDD result;
    std::tuple<ZDD, ZDD> cache_key = std::make_tuple(p, q);
    if (cache.try_get(cache_key, result)) {
        return result;
    }
    // general case (recursion)
    uint32_t x = p_var;
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
    ZDD p0q0 = multiply_impl(p0, q0, cache);
    ZDD p0q1 = multiply_impl(p0, q1, cache);
    ZDD p1q0 = multiply_impl(p1, q0, cache);
    ZDD p1q1 = multiply_impl(p1, q1, cache);
    ZDD tmp = zdd_or(zdd_or(p1q1, p1q0), p0q1);
    result = zdd_makenode(x, p0q0, tmp);
    cache.add(cache_key, result);
    return result;
}

} // namespace

SylvanZddCnf SylvanZddCnf::multiply(const SylvanZddCnf &other) const {
    LruCache<std::tuple<ZDD, ZDD>, ZDD, MULT_CACHE_SIZE, zdd_pair_hash> cache;
    ZDD zdd = multiply_impl(m_zdd, other.m_zdd, cache);
    return SylvanZddCnf(zdd);
}

SylvanZddCnf SylvanZddCnf::filter_literal_in(Literal l) const {
    //ZDD var = zdd_ithvar(literal_to_var(l));
    //ZDD zdd = zdd_exists(m_zdd, var);
    //return SylvanZddCnf(zdd);
    std::vector<Clause> clauses;
    ClauseFunction func = [&](const SylvanZddCnf &, const Clause &clause) {
        if (std::find(clause.cbegin(), clause.cend(), l) != clause.cend()) {
            clauses.push_back(clause);
        }
        return true;
    };
    for_all_clauses(func);
    return from_vector(clauses);
}

SylvanZddCnf SylvanZddCnf::filter_literal_out(Literal l) const {
    std::vector<Clause> clauses;
    ClauseFunction func = [&](const SylvanZddCnf &, const Clause &clause) {
        if (std::find(clause.cbegin(), clause.cend(), l) == clause.cend()) {
            clauses.push_back(clause);
        }
        return true;
    };
    for_all_clauses(func);
    return from_vector(clauses);
}

SylvanZddCnf SylvanZddCnf::resolve_all_pairs(const SylvanZddCnf &other, Literal l) const {
    Literal not_l = -l;
    Var var = literal_to_var(l);
    Var not_var = literal_to_var(not_l);
    ZDD zdd = zdd_false;
    ClauseFunction outer_func = [&](const SylvanZddCnf &, const Clause &clause1) {
        ClauseFunction inner_func = [&](const SylvanZddCnf &, const Clause &clause2) {
            ZDD set1 = set_from_vector(clause1);
            ZDD set2 = set_from_vector(clause2);
            ZDD literals1 = zdd_set_remove(zdd_set_remove(set1, var), not_var);
            ZDD literals2 = zdd_set_remove(zdd_set_remove(set2, var), not_var);
            ZDD domain = zdd_set_union(literals1, literals2);
            size_t domain_size = zdd_set_count(domain);
            std::vector<uint8_t> signs(domain_size, 1);
            ZDD clause = zdd_cube(domain, signs.data(), zdd_true);
            zdd = zdd_or(zdd, clause);
            return true;
        };
        other.for_all_clauses(inner_func);
        return true;
    };
    for_all_clauses(outer_func);
    return SylvanZddCnf(zdd);
}

void SylvanZddCnf::for_all_clauses(ClauseFunction &func) const {
    Clause stack;
    for_all_clauses_impl(func, m_zdd, stack);
}

bool SylvanZddCnf::for_all_clauses_impl(ClauseFunction &func, const ZDD &node, Clause &stack) const {
    if (node == zdd_true) {
        return func(*this, stack);
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

void SylvanZddCnf::print_clauses() const {
    print_clauses(std::cout);
}

void SylvanZddCnf::print_clauses(std::ostream &output) const {
    ClauseFunction func = [&](const SylvanZddCnf &, const Clause &clause) {
        output << "{";
        for (auto &&l : clause) {
            output << " " << l << ",";
        }
        output << "}" << std::endl;
        return true;
    };
    for_all_clauses(func);
}

void SylvanZddCnf::draw_to_file(FILE *file) const {
    zdd_fprintdot(file, m_zdd);
}

void SylvanZddCnf::draw_to_file(const std::string &file_name) const {
    FILE *file = fopen(file_name.c_str(), "w");
    draw_to_file(file);
    fclose(file);
}

const ZDD SylvanZddCnf::get_zdd() const {
    return m_zdd;
}

SylvanZddCnf::Var SylvanZddCnf::literal_to_var(Literal l) {
    assert (l != 0);
    if (l > 0) {
        return 2 * (Var)l;
    } else {
        return 2 * (Var)-l + 1;
    }
}

SylvanZddCnf::Literal SylvanZddCnf::var_to_literal(Var v) {
    auto [q, r] = std::div((Literal)v, 2);
    if (r == 0) {
        return q;
    } else {
        return -q;
    }
}

ZDD SylvanZddCnf::set_from_vector(const Clause &clause) {
    std::vector<Var> vars;
    for (auto &&l : clause) {
        Var v = literal_to_var(l);
        vars.push_back(v);
    }
    ZDD set = zdd_set_from_array(vars.data(), vars.size());
    return set;
}

ZDD SylvanZddCnf::clause_from_vector(const Clause &clause) {
    std::vector<uint8_t> signs(clause.size(), 1);
    ZDD domain = set_from_vector(clause);
    ZDD zdd_clause = zdd_cube(domain, signs.data(), zdd_true);
    return zdd_clause;
}

SylvanZddCnf::Literal SylvanZddCnf::SimpleHeuristic::get_next_literal(const SylvanZddCnf &cnf) {
    Var v = zdd_getvar(cnf.get_zdd());
    return var_to_literal(v);
}

} // namespace dp