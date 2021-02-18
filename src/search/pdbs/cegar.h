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
    int max_refinements,
    int max_pdb_size,
    int max_collection_size,
    bool wildcard_plans,
    double max_time,
    const std::shared_ptr<AbstractTask> &task,
    std::vector<FactPair> &&goals,
    std::unordered_set<int> &&blacklisted_variables,
    const std::shared_ptr<utils::RandomNumberGenerator> &rng,
    utils::Verbosity verbosity);

extern void add_cegar_options_to_parser(
    options::OptionParser &parser);
}

#endif
