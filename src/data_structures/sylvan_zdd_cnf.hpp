#pragma once

#include <vector>
#include <string>
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
    using Var = uint32_t;
    using Literal = int32_t;
    using Clause = std::vector<Literal>;
    using ClauseFunction = std::function<bool(const SylvanZddCnf &, const Clause &)>;
    using Heuristic = SimpleHeuristic;

    SylvanZddCnf();
    SylvanZddCnf(ZDD &zdd);
    SylvanZddCnf(const SylvanZddCnf &other);
    SylvanZddCnf &operator=(const SylvanZddCnf &other);
    ~SylvanZddCnf();

    static Var literal_to_var(Literal l);
    static Literal var_to_literal(Var v);
    static Heuristic get_new_heuristic();

    static SylvanZddCnf from_vector(const std::vector<Clause> &clauses);
    static SylvanZddCnf from_file(const std::string &file_name);
    static SylvanZddCnf unify(const SylvanZddCnf &f, const SylvanZddCnf &g);

    bool is_empty() const;
    bool contains_empty_clause() const;
    SylvanZddCnf unify(const SylvanZddCnf &other) const;
    SylvanZddCnf filter_literal_in(Literal l) const;
    SylvanZddCnf filter_literal_out(Literal l) const;
    SylvanZddCnf resolve_all_pairs(const SylvanZddCnf &other, Var var) const;
    void for_all_clauses(ClauseFunction &func) const;

    void print_clauses() const;
    void print_clauses(std::ostream &output) const;
    void draw_to_file(FILE *file) const;
    void draw_to_file(const std::string &file_name) const;

    const ZDD get_zdd() const;

private:
    //SylvanZddCnf(ZDD &zdd);

    static ZDD set_from_vector(const Clause &clause);
    static ZDD clause_from_vector(const Clause &clause);
    bool for_all_clauses_impl(ClauseFunction &func, const ZDD &node, Clause &stack) const;

    ZDD m_zdd;
    static constexpr Var NEG_OFFSET = 100000; // TODO: make a less arbitrary choice

    class SimpleHeuristic {
    public:
        SylvanZddCnf::Var get_next_variable(const SylvanZddCnf &cnf);
    };
};

} // namespace dp
