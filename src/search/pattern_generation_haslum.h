#ifndef PATTERN_GENERATION_HASLUM_H
#define PATTERN_GENERATION_HASLUM_H

#include <map>
#include <vector>

class Options;
class PDBCollectionHeuristic;
class PDBHeuristic;
class State;
// Implementation of the pattern generation algorithm by Haslum et al.
class PatternGenerationHaslum {
    int pdb_max_size;
    int collection_max_size;
    int num_samples;
    PDBCollectionHeuristic *current_collection;
    // TODO: hash map for map?
    std::map<std::vector<int>, int> pattern_sizes; // cache size of a pattern
    void sample_states(std::vector<State> &samples, int average_operator_costs);
    void generate_candidate_patterns(const std::vector<int> &pattern,
                                     std::vector<std::vector<int> > &candidate_patterns);
    bool counting_approximation(PDBHeuristic *pdbheuristic,
                                const State &sample,
                                PDBCollectionHeuristic *current_collection,
                                std::vector<std::vector<PDBHeuristic *> > &max_additive_subsets);
    void hill_climbing();
public:
    PatternGenerationHaslum(const Options &opts);
    virtual ~PatternGenerationHaslum();
    PDBCollectionHeuristic *get_pattern_collection_heuristic() const { return current_collection; }
};

#endif
