#ifndef PDBS_INCREMENTAL_CANONICAL_PDBS_H
#define PDBS_INCREMENTAL_CANONICAL_PDBS_H

#include "pattern_cliques.h"
#include "pattern_collection_information.h"
#include "types.h"

#include "../task_proxy.h"

#include <memory>

namespace pdbs {
class IncrementalCanonicalPDBs {
    TaskProxy task_proxy;

    std::shared_ptr<PatternCollection> patterns;
    std::shared_ptr<PDBCollection> pattern_databases;
    std::shared_ptr<std::vector<PatternClique>> pattern_cliques;

    // A pair of variables is additive if no operator has an effect on both.
    VariableAdditivity are_additive;

    // The sum of all abstract state sizes of all pdbs in the collection.
    int size;

    // Adds a PDB for pattern but does not recompute pattern_cliques.
    void add_pdb_for_pattern(const Pattern &pattern);

    void recompute_pattern_cliques();
public:
    IncrementalCanonicalPDBs(const TaskProxy &task_proxy,
                             const PatternCollection &intitial_patterns);
    virtual ~IncrementalCanonicalPDBs() = default;

    // Adds a new PDB to the collection and recomputes pattern_cliques.
    void add_pdb(const std::shared_ptr<PatternDatabase> &pdb);

    /* Returns a list of pattern cliques that would be additive to the new
       pattern. Detailed documentation in max_additive_pdb_sets.h */
    std::vector<PatternClique> get_pattern_cliques(const Pattern &new_pattern);

    int get_value(const State &state) const;

    /*
      The following method offers a quick dead-end check for the sampling
      procedure of iPDB-hillclimbing. This exists because we can much more
      efficiently test if the canonical heuristic is infinite than
      computing the exact heuristic value.
    */
    bool is_dead_end(const State &state) const;

    PatternCollectionInformation get_pattern_collection_information() const;

    std::shared_ptr<PDBCollection> get_pattern_databases() const {
        return pattern_databases;
    }

    int get_size() const {
        return size;
    }
};
}

#endif
