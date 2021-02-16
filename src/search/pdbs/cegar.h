#ifndef PDBS_CEGAR_H
#define PDBS_CEGAR_H

#include "pattern_collection_information.h"

#include "../task_proxy.h"

#include "../utils/logging.h"
#include "../utils/rng.h"

#include <unordered_set>

namespace options {
class OptionParser;
}

namespace pdbs {
extern PatternCollectionInformation cegar(
    const std::shared_ptr<AbstractTask> &task,
    std::vector<FactPair> &&goals,
    const std::shared_ptr<utils::RandomNumberGenerator> &rng,
    int max_refinements,
    int max_pdb_size,
    int max_collection_size,
    bool wildcard_plans,
    bool ignore_goal_violations,
    utils::Verbosity verbosity,
    double max_time,
    std::unordered_set<int> &&blacklisted_variables);

extern void add_cegar_options_to_parser(
    options::OptionParser &parser);
}

#endif
