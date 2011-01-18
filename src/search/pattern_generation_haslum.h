#ifndef PATTERN_GENERATION_HASLUM_H
#define PATTERN_GENERATION_HASLUM_H

#include <vector>

class PDBCollectionHeuristic;
class State;
// Implementation of the pattern generation algorithm by Haslum et al.
class PatternGenerationHaslum {
    int max_pdb_memory;
    int samples_number;
    std::vector<const State *> samples;
    PDBCollectionHeuristic *current_collection;
    void sample_states();
    void generate_successors(const PDBCollectionHeuristic &current_collection,
                             std::vector<std::vector<int> > &successor_patterns);
    void generate_successors(std::vector<int> pattern,
                             std::vector<std::vector<int> > &successor_patterns);
    void hill_climbing();
public:
    PatternGenerationHaslum(int max_pdb_memory, int samples_number);
    virtual ~PatternGenerationHaslum();
    PDBCollectionHeuristic *get_pattern_collection_heuristic() { return current_collection; };
};

#endif
