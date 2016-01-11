#ifndef PDBS_TYPES_H
#define PDBS_TYPES_H

#include <memory>
#include <vector>

namespace pdbs {
class PatternDatabase;
using Pattern = std::vector<int>;
using PatternCollection = std::vector<Pattern>;
using PDBCollection = std::vector<std::shared_ptr<PatternDatabase>>;
using MaxAdditivePDBSubsets = std::vector<PDBCollection>;
}

#endif
