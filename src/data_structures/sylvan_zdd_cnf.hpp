#pragma once

#include <vector>
#include <string>
#include <tuple>
#include <iostream>
#include <functional>
#include <cstdio>
#include <sylvan.h>

namespace dp {

using namespace sylvan;

class SylvanZddCnf {
private:
    class SimpleHeuristic;

public:
    using Literal = int32_t;
    using Clause = std::vector<Literal>;
    using ClauseFunction = std::function<bool(const SylvanZddCnf &, const Clause &)>;
    using Heuristic = SimpleHeuristic;

    SylvanZddCnf();
    SylvanZddCnf(ZDD &zdd);
    SylvanZddCnf(const SylvanZddCnf &other);
    SylvanZddCnf &operator=(const SylvanZddCnf &other);
    ~SylvanZddCnf();

    static Heuristic get_new_heuristic();

    static SylvanZddCnf from_vector(const std::vector<Clause> &clauses);
    static SylvanZddCnf from_file(const std::string &file_name);

    bool is_empty() const;
    bool contains_empty_clause() const;
    SylvanZddCnf subset0(Literal l) const;
    SylvanZddCnf subset1(Literal l) const;
    SylvanZddCnf unify(const SylvanZddCnf &other) const;
    SylvanZddCnf intersect(const SylvanZddCnf &other) const;
    SylvanZddCnf subtract(const SylvanZddCnf &other) const;
    SylvanZddCnf multiply(const SylvanZddCnf &other) const;
    void for_all_clauses(ClauseFunction &func) const;

    void print_clauses() const;
    void print_clauses(std::ostream &output) const;
    void draw_to_file(FILE *file) const;
    void draw_to_file(const std::string &file_name) const;

    const ZDD get_zdd() const;

private:
    using Var = uint32_t;

    //SylvanZddCnf(ZDD &zdd);

    static Var literal_to_var(Literal l);
    static Literal var_to_literal(Var v);
    static ZDD set_from_vector(const Clause &clause);
    static ZDD clause_from_vector(const Clause &clause);
    bool for_all_clauses_impl(ClauseFunction &func, const ZDD &node, Clause &stack) const;

    ZDD m_zdd;

    class SimpleHeuristic {
    public:
        SylvanZddCnf::Literal get_next_literal(const SylvanZddCnf &cnf);
    };
};

} // namespace dp
