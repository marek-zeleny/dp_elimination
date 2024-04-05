#pragma once

#include <vector>
#include <string>
#include <tuple>
#include <iostream>
#include <functional>
#include <cstdio>
#include <sylvan.h>

#include "utils.hpp"
#include "data_structures/lru_cache.hpp"

namespace dp {

using namespace sylvan;

class SylvanZddCnf {
public:
    using Literal = int32_t;
    using Clause = std::vector<Literal>;
    using ClauseFunction = std::function<bool(const Clause &)>;

    struct VariableStats {
        size_t positive_clause_count{0};
        size_t negative_clause_count{0};
    };

    struct FormulaStats {
        std::vector<VariableStats> vars;
        size_t index_shift;
    };

    SylvanZddCnf();
    SylvanZddCnf(const SylvanZddCnf &other);
    SylvanZddCnf &operator=(const SylvanZddCnf &other);
    ~SylvanZddCnf();

    static SylvanZddCnf from_vector(const std::vector<Clause> &clauses);
    static SylvanZddCnf from_file(const std::string &file_name);

    inline bool operator==(const SylvanZddCnf &other) const {
        return m_zdd == other.m_zdd;
    }

    [[nodiscard]] size_t count_clauses() const;
    [[nodiscard]] size_t count_nodes() const;
    [[nodiscard]] bool is_empty() const;
    [[nodiscard]] bool contains_empty() const;
    [[nodiscard]] Literal get_smallest_variable() const;
    [[nodiscard]] Literal get_largest_variable() const;
    [[nodiscard]] Literal get_root_literal() const;
    [[nodiscard]] Literal get_unit_literal() const;
    [[nodiscard]] Literal get_clear_literal() const;
    [[nodiscard]] FormulaStats get_formula_statistics() const;
    [[nodiscard]] SylvanZddCnf subset0(Literal l) const;
    [[nodiscard]] SylvanZddCnf subset1(Literal l) const;
    [[nodiscard]] SylvanZddCnf unify(const SylvanZddCnf &other) const;
    [[nodiscard]] SylvanZddCnf intersect(const SylvanZddCnf &other) const;
    [[nodiscard]] SylvanZddCnf subtract(const SylvanZddCnf &other) const;
    [[nodiscard]] SylvanZddCnf multiply(const SylvanZddCnf &other) const;
    [[nodiscard]] SylvanZddCnf remove_tautologies() const;
    [[nodiscard]] SylvanZddCnf remove_subsumed_clauses() const;
    void for_all_clauses(ClauseFunction &func) const;

    [[nodiscard]] std::vector<Clause> to_vector() const;

    void print_clauses(std::ostream &output = std::cout) const;
    void draw_to_file(std::FILE *file) const;
    void draw_to_file(const std::string &file_name) const;
    void write_dimacs_to_file(const std::string &file_name) const;

private:
    using Var = uint32_t;

    ZDD m_zdd;

    explicit SylvanZddCnf(ZDD zdd);

    // helper functions
    static Var literal_to_var(Literal l);
    static Literal var_to_literal(Var v);
    static ZDD set_from_vector(const Clause &clause);
    static ZDD clause_from_vector(const Clause &clause);
    static bool contains_empty_set(const ZDD &zdd);

    // implementations of recursive algorithms
    static ZDD multiply_impl(const ZDD &p, const ZDD &q);
    static ZDD remove_tautologies_impl(const ZDD &zdd);
    static ZDD remove_subsumed_clauses_impl(const ZDD &zdd);
    static ZDD remove_supersets(const ZDD &p, const ZDD &q);
    static bool for_all_clauses_impl(ClauseFunction &func, const ZDD &node, Clause &stack);

    // caching of operation results
    static constexpr size_t CACHE_SIZE = 32;

    using UnaryCacheEntry = ZDD;
    using BinaryCacheEntry = std::tuple<ZDD, ZDD>;
    using UnaryCacheHash = std::hash<UnaryCacheEntry>;
    struct BinaryCacheHash {
        size_t operator()(const BinaryCacheEntry &pair) const {
            size_t seed = 0;
            hash_combine(seed, std::get<0>(pair));
            hash_combine(seed, std::get<1>(pair));
            return seed;
        }
    };
    using UnaryCache = LruCache<UnaryCacheEntry, ZDD, CACHE_SIZE, UnaryCacheHash>;
    using BinaryCache = LruCache<BinaryCacheEntry, ZDD, CACHE_SIZE, BinaryCacheHash>;

    inline static BinaryCache s_multiply_cache;
    inline static UnaryCache s_remove_tautologies_cache;
    inline static UnaryCache s_remove_subsumed_clauses_cache;
    inline static BinaryCache s_remove_supersets_cache;
};

} // namespace dp
