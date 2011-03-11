#ifndef PATTERN_GENERATION_HASLUM_H
#define PATTERN_GENERATION_HASLUM_H

#include <ext/hash_map>
#include <map>
#include <vector>

class PDBCollectionHeuristic;
class State;
class PDBHeuristic;
// Implementation of the pattern generation algorithm by Haslum et al.
class PatternGenerationHaslum {
    int max_pdb_size;
    int max_collection_size;
    int num_samples;
    PDBCollectionHeuristic *current_collection;
    std::map<std::vector<int>, PDBHeuristic *> pattern_to_pdb; // cache pdbs to avoid recalculation
    void sample_states(std::vector<State> &samples);
    /*void initial_candidates(const PDBCollectionHeuristic &current_collection,
                             std::vector<std::vector<int> > &candidate_patterns);*/
    void generate_candidate_patterns(const std::vector<int> &pattern,
                                     std::vector<std::vector<int> > &candidate_patterns);
    bool counting_approximation(PDBHeuristic *pdbheuristic,
                                const State &sample,
                                PDBCollectionHeuristic *current_collection,
                                std::vector<std::vector<PDBHeuristic *> > &max_additive_subsets);
    void hill_climbing();
public:
    PatternGenerationHaslum(int max_pdb_size, int max_collection_size, int num_samples);
    virtual ~PatternGenerationHaslum();
    PDBCollectionHeuristic *get_pattern_collection_heuristic() const { return current_collection; }
};

#endif
