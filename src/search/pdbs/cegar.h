#ifndef PDBS_CEGAR_H
#define PDBS_CEGAR_H

#include "pattern_collection_information.h"

#include "../task_proxy.h"

#include "../utils/logging.h"
#include "../utils/rng.h"

namespace options {
class OptionParser;
}

namespace utils {
class CountdownTimer;
}

namespace pdbs {
class AbstractSolutionData;

struct Flaw {
    int solution_index;
    int variable;

    Flaw(int solution_index, int variable)
            : solution_index(solution_index),
              variable(variable) {
    }
};

using FlawList = std::vector<Flaw>;

enum class InitialCollectionType {
    GIVEN_GOAL,
    RANDOM_GOAL,
    ALL_GOALS
};

extern PatternCollectionInformation cegar(
    const std::shared_ptr<AbstractTask> &task,
    const std::shared_ptr<utils::RandomNumberGenerator> &rng,
    int max_refinements,
    int max_pdb_size,
    int max_collection_size,
    bool wildcard_plans,
    bool ignore_goal_violations,
    int global_blacklist_size,
    InitialCollectionType initial,
    int given_goal,
    utils::Verbosity verbosity,
    double max_time);

extern void add_cegar_options_to_parser(
    options::OptionParser &parser);
}

#endif