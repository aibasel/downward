#ifndef PDBS_CEGAR_H
#define PDBS_CEGAR_H

#include "pattern_collection_information.h"
#include "pattern_database.h"
#include "pattern_information.h"

#include <memory>
#include <vector>

namespace plugins {
class Feature;
}

namespace utils {
class LogProxy;
class RandomNumberGenerator;
}

namespace pdbs {
/*
  This function implements the CEGAR algorithm for computing disjoint pattern
  collections.

  In a nutshell, it receives a concrete task plus a (sub)set of its goals in a
  randomized order. Starting from the pattern collection consisting of a
  singleton pattern for each goal variable, it repeatedly attempts to execute
  an optimal plan of each pattern in the concrete task, collects reasons why
  this is not possible (so-called flaws) and refines the pattern in question
  by adding a variable to it.

  Further parameters allow blacklisting a (sub)set of the non-goal variables
  which are then never added to the collection, limiting PDB and collection
  size, setting a time limit and switching between computing regular or
  wildcard plans, where the latter are sequences of parallel operators
  inducing the same abstract transition.
*/
extern PatternCollectionInformation generate_pattern_collection_with_cegar(
    int max_pdb_size,
    int max_collection_size,
    double max_time,
    bool use_wildcard_plans,
    utils::LogProxy &log,
    const std::shared_ptr<utils::RandomNumberGenerator> &rng,
    const std::shared_ptr<AbstractTask> &task,
    const std::vector<FactPair> &goals,
    std::unordered_set<int> &&blacklisted_variables = std::unordered_set<int>());

/*
  This function implements the CEGAR algorithm as described above, however
  restricted to a single goal and therefore guaranteed to generate a single
  pattern instead of a pattern collection.
*/
extern PatternInformation generate_pattern_with_cegar(
    int max_pdb_size,
    double max_time,
    bool use_wildcard_plans,
    utils::LogProxy &log,
    const std::shared_ptr<utils::RandomNumberGenerator> &rng,
    const std::shared_ptr<AbstractTask> &task,
    const FactPair &goal,
    std::unordered_set<int> &&blacklisted_variables = std::unordered_set<int>());

extern void add_cegar_implementation_notes_to_feature(plugins::Feature &feature);
extern void add_cegar_wildcard_option_to_feature(plugins::Feature &feature);
}

#endif
