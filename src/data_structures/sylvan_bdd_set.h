#pragma once

#include <sylvan_obj.hpp>

using namespace sylvan;

namespace dp {

class SylvanBddSet {
    using Set = Sylvan::BddSet;
    using Var = uint32_t;

    Set set;

public:
    SylvanBddSet() : set(Set()) {}

    SylvanBddSet(const std::vector<Var> &variables)
        : set(Set::fromVector(variables)) {}

    SylvanBddSet(const SylvanBddSet &other) : set(other.set) {}

    SylvanBddSet(const Set &source) : set(source) {}

    bool is_empty() const {
        return set.IsEmpty();
    }

    bool contains(Var variable) const {
        return set.contains(variable);
    }

    size_t size() const {
        return set.size();
    }

    void add(Var variable) {
        set.add(variable);
    }

    void add(SylvanBddSet &other) {
        set.add(other.set);
    }

    void remove(Var variable) {
        set.remove(variable);
    }

    void remove(SylvanBddSet &other) {
        set.remove(other.set);
    }

    std::vector<Var> to_vector() const {
        return set.toVector();
    }
};

} // namespace dp
