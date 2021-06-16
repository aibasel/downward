#ifndef PDBS_PATTERN_GENERATOR_RCG_H
#define PDBS_PATTERN_GENERATOR_RCG_H

#include "pattern_generator.h"

#include <memory>

namespace utils {
class RandomNumberGenerator;
}

namespace pdbs {
class PatternGeneratorRCG {
    std::shared_ptr<utils::RandomNumberGenerator> rng;
    std::vector<std::vector<int>> &neighborhoods;
    int max_pdb_size;
    int start_goal_variable;
    bool can_merge(const TaskProxy &tp, const Pattern &P, int new_var);
public:
    PatternGeneratorRCG(
            std::shared_ptr<utils::RandomNumberGenerator> arg_rng,
            std::vector<std::vector<int>> &arg_neighborhoods,
            int arg_max_pdb_size,
            int start_goal_variable);
    virtual ~PatternGeneratorRCG() = default;
    Pattern generate(
            const std::shared_ptr<AbstractTask> &task);
};
}


#endif
