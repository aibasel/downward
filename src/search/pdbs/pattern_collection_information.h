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
*/
class PatternCollectionInformation {
    TaskProxy task_proxy;
    std::shared_ptr<PatternCollection> patterns;
    std::shared_ptr<PDBCollection> pdbs;
    std::shared_ptr<MaxAdditivePDBSubsets> max_additive_subsets;

    void create_pdbs_if_missing();
    void create_max_additive_subsets_if_missing();

    bool information_is_valid() const;
public:
    PatternCollectionInformation(
        const TaskProxy &task_proxy,
        const std::shared_ptr<PatternCollection> &patterns);
    ~PatternCollectionInformation() = default;

    void set_pdbs(const std::shared_ptr<PDBCollection> &pdbs);
    void set_max_additive_subsets(
        const std::shared_ptr<MaxAdditivePDBSubsets> &max_additive_subsets);

    std::shared_ptr<PatternCollection> get_patterns();
    std::shared_ptr<PDBCollection> get_pdbs();
    std::shared_ptr<MaxAdditivePDBSubsets> get_max_additive_subsets();
};
}

#endif
