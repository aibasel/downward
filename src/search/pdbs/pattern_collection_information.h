#ifndef PDBS_PATTERN_COLLECTION_INFORMATION_H
#define PDBS_PATTERN_COLLECTION_INFORMATION_H

#include "types.h"

#include "../task_proxy.h"

#include <memory>


// TODO issue585: better documentation
/*
  The main goal of this class is to "transport" patterns, PDBs and PDB-cliques
  from one class to another. It uses shared pointers, so its users can extract
  the information they want, and then destroy the PatternCollection.
  Missing information (PDBs, cliques) is computed on demand.
  Class invariants:
    * All patterns are sorted.
    * The list of patterns is sorted and non-null.
    * If cliques is non-null, then pdbs is non-null.
    * The shared pointers used in cliques are shared with pdbs.
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
