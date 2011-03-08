#ifndef PATTERN_GENERATION_EDELKAMP_H
#define PATTERN_GENERATION_EDELKAMP_H

#include <ext/hash_map>
#include <map>
#include <vector>

class PDBCollectionHeuristic;
// Implementation of the pattern generation algorithm by Edelkamp
class PatternGenerationEdelkamp {
    int pdb_max_size;
    int num_collections;
    // TODO: check why it doesn't work with hash_map
    //__gnu_cxx::hash_map<std::vector<bool>, double> pattern_to_fitness;
    std::map<std::vector<bool>, double> pattern_to_fitness; // cache fitness values for already calculated patterns
    std::vector<std::vector<std::vector<bool> > > pattern_collections; // all current pattern collections
    std::vector<std::vector<bool> > best_collection; // remember the best pattern collection over all episodes
    void initialize(); // bin packing (for variables) to determine initial pattern collections
    //void recombine();
    void mutate(double probability); // flip bits (= variables) with a given probability

    // calculate the mean h-value (fitness function) for each pattern collection and returns the sum of them
    double evaluate(std::vector<std::pair<double, int> > &fitness_values);

    // select each pattern collection with a probability (obtained by normalizing the fitness function)
    void select(const std::vector<std::pair<double, int> > &fitness_values, double fitness_sum);

    // transforms a vector of bools (pattern representation in this algorithm) to the "normal" pattern form
    // vector<int> (needed for PDBHeuristic)
    void transform_to_pattern_normal_form(const std::vector<bool> &bitvector, std::vector<int> &pattern) const;
public:
    PatternGenerationEdelkamp(int pdb_max_size, int num_collections, int num_episodes,
                              double mutation_probability);
    virtual ~PatternGenerationEdelkamp();
    void dump() const;

    // TODO: make const again after getting rid of call to evaluate in this method
    PDBCollectionHeuristic *get_pattern_collection_heuristic();
};

#endif
