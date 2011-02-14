#ifndef PATTERN_GENERATION_EDELKAMP_H
#define PATTERN_GENERATION_EDELKAMP_H

class PDBCollectionHeuristic;
// Implementation of the pattern generation algorithm by Edelkamp et al.
class PatternGenerationEdelkamp {
    PDBCollectionHeuristic *current_collection;
    void initialize(int initial_pdb_max_size);
public:
    PatternGenerationEdelkamp(int initial_pdb_max_size);
    virtual ~PatternGenerationEdelkamp();
    PDBCollectionHeuristic *get_pattern_collection_heuristic() { return current_collection; }
};

#endif
