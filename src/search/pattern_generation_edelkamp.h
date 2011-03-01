#ifndef PATTERN_GENERATION_EDELKAMP_H
#define PATTERN_GENERATION_EDELKAMP_H

#include <vector>

class PDBCollectionHeuristic;
// Implementation of the pattern generation algorithm by Edelkamp
class PatternGenerationEdelkamp {
    int pdb_max_size;
    int num_collections;
    std::vector<std::vector<std::vector<bool> > > pattern_collections; // all current pattern collections
    //std::vector<std::vector<bool> > best_collection; // remember the best pattern collection over all episodes
    void initialize(); // bin packing (for variables) to determine initial pattern collections
    //void recombine();
    void mutate(double probability); // flip bits (= variables) with a given probability

    // calculate the mean h-value (fitness function) for each pattern collection
    void evaluate(std::vector<double> &fitness_values) const;

    // select each pattern collection with a probability (obtained by normalizing the fitness function)
    void select(const std::vector<double> &fitness_values);

    // transforms a vector of bools (pattern representation in this algorithm) to the "normal" pattern form
    // vector<int> (needed for PDBHeuristic)
    void transform_to_pattern_normal_form(const std::vector<bool> &bitvector, std::vector<int> &pattern) const;
public:
    PatternGenerationEdelkamp(int pdb_max_size, int num_collections, int num_episodes,
                              double mutation_probability);
    virtual ~PatternGenerationEdelkamp();
    void dump() const;

    PDBCollectionHeuristic *get_pattern_collection_heuristic() const;
};

#endif
