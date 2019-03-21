#ifndef PDBS_PATTERN_COLLECTION_INFORMATION_H
#define PDBS_PATTERN_COLLECTION_INFORMATION_H

#include "types.h"

#include "../task_proxy.h"

#include <memory>

namespace pdbs {
/*
  This class contains everything we know about a pattern collection. It will
  always contain patterns, but can also contain the computed PDBs and maximal
  additive subsets of the PDBs. If one of the latter is not available, then
  this information is created when it is requested.
  Ownership of the information is shared between the creators of this class
  (usually PatternCollectionGenerators), the class itself, and its users
  (consumers of pattern collections like heuristics).

  TODO: this should probably re-use PatternInformation and it could also act
  as an interface for ownership transfer rather than sharing it.
*/
class PatternCollectionInformation {
    TaskProxy task_proxy;
    std::shared_ptr<PatternCollection> patterns;
    std::shared_ptr<PDBCollection> pdbs;
    std::shared_ptr<std::vector<PatternClique>> pattern_cliques;

    void create_pdbs_if_missing();
    void create_pattern_cliques_if_missing();

    bool information_is_valid() const;
public:
    PatternCollectionInformation(
        const TaskProxy &task_proxy,
        const std::shared_ptr<PatternCollection> &patterns);
    ~PatternCollectionInformation() = default;

    void set_pdbs(const std::shared_ptr<PDBCollection> &pdbs);
    void set_pattern_cliques(
        const std::shared_ptr<std::vector<PatternClique>> &pattern_cliques);

    TaskProxy get_task_proxy() const {
        return task_proxy;
    }

    std::shared_ptr<PatternCollection> get_patterns() const;
    std::shared_ptr<PDBCollection> get_pdbs();
    std::shared_ptr<std::vector<PatternClique>> get_pattern_cliques();
};
}

#endif
