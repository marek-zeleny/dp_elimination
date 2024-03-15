#pragma once

#include <type_traits>
#include <simple_logger.h>
#include "data_structures/sylvan_zdd_cnf.hpp"

namespace dp {

template<typename T>
concept IsHeuristic = requires(T t, const SylvanZddCnf &cnf) {
    { t.get_next_literal(cnf) } -> std::same_as<SylvanZddCnf::Literal>;
};

class SimpleHeuristic {
public:
    SylvanZddCnf::Literal get_next_literal(const SylvanZddCnf &cnf) {
        SylvanZddCnf::Literal l = cnf.get_root_literal();
        LOG_INFO << "Heuristic found root literal " << l;
        return l;
    }
};

class UnitLiteralHeuristic {
public:
    SylvanZddCnf::Literal get_next_literal(const SylvanZddCnf &cnf) {
        SylvanZddCnf::Literal l = cnf.get_unit_literal();
        if (l == 0) {
            l = cnf.get_root_literal();
            LOG_INFO << "Heuristic didn't find any unit literal, returning root literal " << l << " instead";
        } else {
            LOG_INFO << "Heuristic found unit literal " << l;
        }
        return l;
    }
};

class ClearLiteralHeuristic {
public:
    SylvanZddCnf::Literal get_next_literal(const SylvanZddCnf &cnf) {
        SylvanZddCnf::Literal l = cnf.get_clear_literal();
        if (l == 0) {
            l = cnf.get_root_literal();
            LOG_INFO << "Heuristic didn't find any clear literal, returning root literal " << l << " instead";
        } else {
            LOG_INFO << "Heuristic found clear literal " << l;
        }
        return l;
    }
};

} // namespace dp
