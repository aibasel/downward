#ifndef PDBS_PATTERN_COLLECTION_INFORMATION_H
#define PDBS_PATTERN_COLLECTION_INFORMATION_H

#include "types.h"

#include "../task_proxy.h"

#include <memory>


/*
  This class contains everything we know about a pattern collection. It will
  always contain patterns, but can also contain the computed PDBs and maximal
  additive subsets of the PDBs. If one of the latter is not available, then
  this information is created when it is requested.
  Ownership of the information is shared between the creators of this class
  (usually PatternCollectionGenerators), the class itself, and its users
  (consumers of pattern collections like heuristics).

  Class invariants:
    * All patterns are sorted.
    * The list of patterns is sorted and non-null.
    * If max_additive_subsets is non-null, then pdbs is non-null.
    * The shared pointers used in max_additive_subsets are shared with pdbs.
*/
class PatternCollectionInformation {
    std::shared_ptr<AbstractTask> task;
    TaskProxy task_proxy;
    std::shared_ptr<PatternCollection> patterns;
    std::shared_ptr<PDBCollection> pdbs;
    std::shared_ptr<MaxAdditivePDBSubsets> max_additive_subsets;

    void create_pdbs_if_missing();
    void create_max_additive_subsets_if_missing();
public:
    PatternCollectionInformation(
        std::shared_ptr<AbstractTask> task,
        std::shared_ptr<PatternCollection> patterns);
    PatternCollectionInformation(
        std::shared_ptr<AbstractTask> task,
        std::shared_ptr<MaxAdditivePDBSubsets> max_additive_subsets);
    ~PatternCollectionInformation() = default;

    std::shared_ptr<PatternCollection> get_patterns();
    std::shared_ptr<PDBCollection> get_pdbs();
    std::shared_ptr<MaxAdditivePDBSubsets> get_max_additive_subsets();
};

#endif
