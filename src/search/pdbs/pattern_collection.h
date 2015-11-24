#ifndef PDBS_PATTERN_COLLECTION_H
#define PDBS_PATTERN_COLLECTION_H

#include "types.h"

#include "../task_proxy.h"

#include <memory>


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
class PatternCollection {
    std::shared_ptr<AbstractTask> task;
    TaskProxy task_proxy;
    std::shared_ptr<Patterns> patterns;
    std::shared_ptr<PDBCollection> pdbs;
    std::shared_ptr<PDBCliques> cliques;

    void create_pdbs_if_missing();
    void create_cliques_if_missing();
public:
    PatternCollection(
        std::shared_ptr<AbstractTask> task,
        std::shared_ptr<Patterns> patterns);
    PatternCollection(
        std::shared_ptr<AbstractTask> task,
        std::shared_ptr<PDBCliques> cliques);
    ~PatternCollection();

    std::shared_ptr<Patterns> get_patterns();
    std::shared_ptr<PDBCollection> get_pdbs();
    std::shared_ptr<PDBCliques> get_cliques();
};

#endif
