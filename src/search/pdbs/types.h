#ifndef PDBS_TYPES_H
#define PDBS_TYPES_H

#include <memory>
#include <vector>

class PatternDatabase;
using Pattern = std::vector<int>;
using Patterns = std::vector<Pattern>;
using PDBCollection = std::vector<std::shared_ptr<PatternDatabase>>;
using PDBCliques = std::vector<PDBCollection>;

#endif
