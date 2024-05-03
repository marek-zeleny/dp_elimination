#pragma once

#include <vector>
#include <string>
#include <tuple>
#include <deque>
#include <iostream>
#include <functional>
#include <memory>
#include <cstdio>

#include "utils.hpp"
#include "data_structures/lru_cache.hpp"

namespace dp {

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

    struct SylvanStats {
        size_t table_filled;
        size_t table_total;
    };

    static SylvanStats get_sylvan_stats();
    static void call_sylvan_gc();
    static void hook_sylvan_gc_log();

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
    [[nodiscard]] size_t count_depth() const;
    [[nodiscard]] bool is_empty() const;
    [[nodiscard]] bool contains_empty() const;
    [[nodiscard]] bool contains_unit_literal(const Literal &literal) const;
    [[nodiscard]] Literal get_smallest_variable() const;
    [[nodiscard]] Literal get_largest_variable() const;
    [[nodiscard]] Literal get_root_literal() const;
    [[nodiscard]] Literal get_unit_literal() const;
    [[nodiscard]] Literal get_clear_literal() const;
    [[nodiscard]] FormulaStats get_formula_statistics() const;
    [[nodiscard]] SylvanZddCnf subset0(Literal l) const;
    [[nodiscard]] SylvanZddCnf subset1(Literal l) const;
    [[nodiscard]] SylvanZddCnf unify(const SylvanZddCnf &other) const;
    [[nodiscard]] SylvanZddCnf unify_and_remove_subsumed(const SylvanZddCnf &other) const;
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

    [[nodiscard]] bool verify_variable_ordering() const;

private:
    using ZDD = uint64_t;
    using Var = uint32_t;

    ZDD m_zdd;

    explicit SylvanZddCnf(ZDD zdd);

    // logarithmic building
    class LogarithmicBuilder {
    public:
        LogarithmicBuilder();
        ~LogarithmicBuilder();

        void add_clause(const Clause &c);
        [[nodiscard]] ZDD get_result() const;
        [[nodiscard]] size_t get_size() const;

    private:
        using entry = std::tuple<ZDD, size_t>;
        static constexpr size_t unit_size = 1024;
        std::deque<entry> m_forest;

        void check_and_merge();
    };

    // helper functions
    static Var literal_to_var(Literal l);
    static Literal var_to_literal(Var v);
    static ZDD set_from_vector(const Clause &clause);
    static ZDD clause_from_vector(const Clause &clause);
    static bool contains_empty_set(const ZDD &zdd);

    // implementations of recursive algorithms
    static bool for_all_clauses_impl(ClauseFunction &func, const ZDD &node, Clause &stack);

    // caching of operation results
    static constexpr size_t CACHE_SIZE = 0;

    using Zdd_ptr = std::shared_ptr<ZDD>;
    using UnaryCacheKey = Zdd_ptr;
    using BinaryCacheKey = std::tuple<Zdd_ptr, Zdd_ptr>;
    struct UnaryCacheHash {
        static inline std::hash<ZDD> hash{};
        size_t operator()(const UnaryCacheKey &key) const {
            return hash(*key);
        }
    };
    struct BinaryCacheHash {
        size_t operator()(const BinaryCacheKey &pair) const {
            size_t seed = 0;
            hash_combine(seed, *std::get<0>(pair));
            hash_combine(seed, *std::get<1>(pair));
            return seed;
        }
    };
    using UnaryCache = LruCache<UnaryCacheKey, Zdd_ptr, CACHE_SIZE, UnaryCacheHash>;
    using BinaryCache = LruCache<BinaryCacheKey, Zdd_ptr, CACHE_SIZE, BinaryCacheHash>;

    static void store_in_unary_cache(UnaryCache &cache, const ZDD &key, const ZDD &entry);
    static void store_in_binary_cache(BinaryCache &cache, const ZDD &key1, const ZDD &key2, const ZDD &entry);
    static std::optional<ZDD> try_get_from_unary_cache(UnaryCache &cache, const ZDD &key);
    static std::optional<ZDD> try_get_from_binary_cache(BinaryCache &cache, const ZDD &key1, const ZDD &key2);
};

} // namespace dp
