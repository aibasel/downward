#ifndef PARTIAL_RELAXATION_H
#define PARTIAL_RELAXATION_H

#include <vector>

class PartialRelaxation {
    std::vector<bool> var_is_relaxed;
    std::vector<int> var_to_index;
    int max_relaxed_index;
    int max_unrelaxed_index;
public:
    PartialRelaxation(const std::vector<int> &relaxed_variables);
    ~PartialRelaxation();

    bool is_relaxed(int var_no) const;
    int get_relaxed_index(int var_no, int value) const;
    int get_unrelaxed_index(int var_no) const;

    int get_max_relaxed_index() const;
    int get_max_unrelaxed_index() const;
};

#endif
