#ifndef PDBS_PATTERN_INFORMATION_H
#define PDBS_PATTERN_INFORMATION_H

#include "types.h"

#include "../task_proxy.h"

#include <memory>

namespace pdbs {
/*
  This class is a wrapper for a pair of a pattern and the corresponding PDB.
  It always contains a pattern and can contain the computed PDB. If the latter
  is not set, it can be computed on demand.
  Ownership of the information is shared between the creators of this class
  (usually PatternGenerators), the class itself, and its users (consumers of
  patterns like heuristics).
*/
class PatternInformation {
    TaskProxy task_proxy;
    std::shared_ptr<Pattern> pattern;
    std::shared_ptr<PatternDatabase> pdb;

    void create_pdb_if_missing(bool dump);

    bool information_is_valid() const;
public:
    PatternInformation(
        const TaskProxy &task_proxy,
        const std::shared_ptr<Pattern> &pattern);
    ~PatternInformation() = default;

    void set_pdb(const std::shared_ptr<PatternDatabase> &pdb);

    std::shared_ptr<Pattern> get_pattern();
    std::shared_ptr<PatternDatabase> get_pdb(bool dump=false);
};
}

#endif
