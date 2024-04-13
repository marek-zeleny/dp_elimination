#include "vector_cnf.hpp"

#include <ranges>
#include <algorithm>
#include <stdexcept>
#include <simple_logger.h>

#include "io/cnf_reader.hpp"

namespace dp {

VectorCnf::VectorCnf() : m_clauses() {}

VectorCnf::VectorCnf(std::vector<Clause> &&clauses) : m_clauses(std::move(clauses)) {}

VectorCnf VectorCnf::from_vector(const std::vector<Clause> &clauses) {
    std::vector<Clause> result;
    for (Clause c : clauses) {
        std::ranges::sort(c);
        result.push_back(std::move(c));
    }
    std::ranges::sort(result);
    return VectorCnf{std::move(result)};
}

VectorCnf VectorCnf::from_file(const std::string &file_name) {
    std::vector<Clause> clauses;
    CnfReader::AddClauseFunction func = [&clauses](CnfReader::Clause c) {
        std::ranges::sort(c);
        clauses.push_back(std::move(c));
    };
    try {
        CnfReader::read_from_file(file_name, func);
    } catch (const CnfReader::failure &f) {
        LOG_ERROR << f.what();
        throw;
    }
    std::ranges::sort(clauses);
    return VectorCnf{std::move(clauses)};
}

size_t VectorCnf::count_clauses() const {
    return m_clauses.size();
}

bool VectorCnf::is_empty() const {
    return m_clauses.empty();
}

bool VectorCnf::contains_empty() const {
    return !is_empty() && m_clauses[0].empty();
}

VectorCnf &VectorCnf::subset0(Literal l) {
    std::erase_if(m_clauses, [&l](const Clause &c) {
        return std::ranges::find(c, l) != c.end();
    });
    return *this;
}

VectorCnf &VectorCnf::subset1(Literal l) {
    std::erase_if(m_clauses, [&l](Clause &c) {
        auto it = std::ranges::find(c, l);
        if (it == c.end()) {
            return true;
        } else {
            c.erase(it);
            return false;
        }
    });
    std::ranges::sort(m_clauses);
    return *this;
}

VectorCnf &VectorCnf::unify(const VectorCnf &other) {
    for (const Clause &c : other.m_clauses) {
        if (!std::ranges::binary_search(m_clauses, c)) {
            m_clauses.push_back(c);
        }
    }
    std::ranges::sort(m_clauses);
    return *this;
}

VectorCnf &VectorCnf::intersect(const VectorCnf &other) {
    std::erase_if(m_clauses, [&other](const Clause &c) {
        return !std::ranges::binary_search(other.m_clauses, c);
    });
    return *this;
}

VectorCnf &VectorCnf::subtract(const VectorCnf &other) {
    std::erase_if(m_clauses, [&other](const Clause &c) {
        return std::ranges::binary_search(other.m_clauses, c);
    });
    return *this;
}

VectorCnf &VectorCnf::multiply(const VectorCnf &other) {
    std::vector<Clause> result;
    for (Clause c1 : m_clauses) {
        for (const Clause &c2 : other.m_clauses) {
            c1.insert(c1.end(), c2.begin(), c2.end());
            std::ranges::sort(c1);
            auto rest = std::ranges::unique(c1);
            c1.erase(rest.begin(), rest.end());
            if (std::ranges::find(result, c1) == result.end()) {
                result.push_back(std::move(c1));
            }
        }
    }
    std::ranges::sort(result);
    m_clauses = std::move(result);
    return *this;
}

VectorCnf &VectorCnf::remove_tautologies() {
    std::erase_if(m_clauses, [](const Clause &c){
        for (Literal l : c) {
            if (l > 0) {
                return false;
            } else if (std::ranges::binary_search(c, -l)) {
                return true;
            }
        }
        return false;
    });
    return *this;
}

VectorCnf &VectorCnf::remove_subsumed_clauses() {
    throw std::logic_error("Method not implemented");
}

void VectorCnf::for_all_clauses(ClauseFunction &func) const {
    for (const Clause &clause : m_clauses) {
        if (!func(clause)) {
            return;
        }
    }
}

std::vector<VectorCnf::Clause> VectorCnf::to_vector() const {
    return m_clauses;
}

} // namespace dp
