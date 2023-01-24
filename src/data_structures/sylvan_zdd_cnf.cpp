#include "data_structures/sylvan_zdd_cnf.hpp"

#include <algorithm>
#include <cstdio>
#include <cassert>
#include <sylvan.h>
#include "sylvan_zdd_cnf.hpp"

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

SylvanZddCnf::Var SylvanZddCnf::literal_to_var(Literal l) {
    assert (l != 0);
    if (l > 0) {
        return (Var)l;
    } else {
        return (Var)-l + NEG_OFFSET;
    }
}

SylvanZddCnf::Literal SylvanZddCnf::var_to_literal(Var v) {
    if (v < NEG_OFFSET) {
        return (Literal)v;
    } else {
        return -(Literal)(v - NEG_OFFSET);
    }
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

SylvanZddCnf SylvanZddCnf::unify(const SylvanZddCnf &f, const SylvanZddCnf &g) {
    ZDD zdd = zdd_or(f.m_zdd, g.m_zdd);
    return SylvanZddCnf(zdd);
}

bool SylvanZddCnf::is_empty() const {
    return m_zdd == zdd_false;
}

bool SylvanZddCnf::contains_empty_clause() const {
    return (m_zdd & zdd_complement) != 0;
}

SylvanZddCnf SylvanZddCnf::unify(const SylvanZddCnf &other) const {
    return unify(*this, other);
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

SylvanZddCnf SylvanZddCnf::resolve_all_pairs(const SylvanZddCnf &other, Var var) const {
    Literal l = var_to_literal(var);
    Literal not_l = -l;
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

SylvanZddCnf::Var SylvanZddCnf::SimpleHeuristic::get_next_variable(const SylvanZddCnf &cnf) {
    Var v = zdd_getvar(cnf.get_zdd());
    Literal l = var_to_literal(v);
    return literal_to_var(std::abs(l));
}

} // namespace dp