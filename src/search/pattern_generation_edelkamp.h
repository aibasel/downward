#ifndef PATTERN_GENERATION_EDELKAMP_H
#define PATTERN_GENERATION_EDELKAMP_H

#include <vector>

class PDBCollectionHeuristic;
// Implementation of the pattern generation algorithm by Edelkamp
class PatternGenerationEdelkamp {
    int max_collection_number;
    std::vector<std::vector<std::vector<bool> > > pattern_collections;
    void initialize(int initial_pdb_max_size); // bin packing with variables to determine
    // initial pattern collections of a give size
    void recombine();
    void mutate(); // flip bits (= variables) with a small probability
    void evaluate(std::vector<std::pair<int, int> > &fitness_values) const; // calculate the mean h-value
    // (fitness function) for each pattern collection
    void select(const std::vector<std::pair<int, int> > &fitness_values); // select each pattern collection
    // with a probability according to its fitness function value
public:
    PatternGenerationEdelkamp(int initial_pdb_max_size, int max_collection_number, int num_episodes);
    virtual ~PatternGenerationEdelkamp();
    void dump() const;
    PDBCollectionHeuristic *get_pattern_collection_heuristic() const;
};

#endif
