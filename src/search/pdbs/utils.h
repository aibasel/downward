#ifndef PDBS_UTILS_H
#define PDBS_UTILS_H

#include "types.h"

#include "../task_proxy.h"

#include "../utils/timer.h"

#include <memory>
#include <string>

namespace utils {
class LogProxy;
class RandomNumberGenerator;
}

namespace pdbs {
class PatternCollectionInformation;
class PatternInformation;

extern int compute_pdb_size(const TaskProxy &task_proxy, const Pattern &pattern);
extern int compute_total_pdb_size(
    const TaskProxy &task_proxy, const PatternCollection &pattern_collection);
extern bool is_operator_relevant(const Pattern &pattern, const OperatorProxy &op);

extern std::vector<FactPair> get_goals_in_random_order(
    const TaskProxy &task_proxy, utils::RandomNumberGenerator &rng);
extern std::vector<int> get_non_goal_variables(const TaskProxy &task_proxy);

/*
  Compute the causal graph neighbors for each variable of the task. If
  bidirectional is false, then only predecessors of variables are considered
  neighbors. If bidirectional is true, then the causal graph is treated as
  undirected graph and also successors of variables are considered neighbors.
*/
extern std::vector<std::vector<int>> compute_cg_neighbors(
    const std::shared_ptr<AbstractTask> &task,
    bool bidirectional);

extern PatternCollectionInformation get_pattern_collection_info(
    const TaskProxy &task_proxy,
    const std::shared_ptr<PDBCollection> &pdbs,
    utils::LogProxy &log);

/*
  Dump the given pattern, the number of variables contained, the size of the
  corresponding PDB, and the runtime used for computing it. All output is
  prepended with the given string identifier.
*/
extern void dump_pattern_generation_statistics(
    const std::string &identifier,
    utils::Duration runtime,
    const PatternInformation &pattern_info,
    utils::LogProxy &log);

/*
  Compute and dump the number of patterns, the total size of the corresponding
  PDBs, and the runtime used for computing the collection. All output is
  prepended with the given string identifier.
*/
extern void dump_pattern_collection_generation_statistics(
    const std::string &identifier,
    utils::Duration runtime,
    const PatternCollectionInformation &pci,
    utils::LogProxy &log);

extern std::string get_rovner_et_al_reference();
}

#endif
