#ifndef PATTERN_GENERATION_EDELKAMP_H
#define PATTERN_GENERATION_EDELKAMP_H

#include <vector>

class PDBCollectionHeuristic;
// Implementation of the pattern generation algorithm by Edelkamp et al.
class PatternGenerationEdelkamp {
    int max_collection_number;
    std::vector<std::vector<std::vector<bool> > > pattern_collections;
    void initialize(int initial_pdb_max_size);
    void recombine();
    void mutate();
    void evaluate(std::vector<std::pair<int, int> > &fitness_values) const;
    void select(const std::vector<std::pair<int, int> > &fitness_values);
public:
    PatternGenerationEdelkamp(int initial_pdb_max_size, int max_collection_number);
    virtual ~PatternGenerationEdelkamp();
    void dump() const;
    PDBCollectionHeuristic *get_pattern_collection_heuristic() const;
};

#endif
