#ifndef PDBS_PATTERN_COLLECTION_GENERATOR_GENETIC_H
#define PDBS_PATTERN_COLLECTION_GENERATOR_GENETIC_H

#include "pattern_generator.h"
#include "types.h"

#include <memory>
#include <vector>

class AbstractTask;

namespace utils {
class RandomNumberGenerator;
}

namespace pdbs {
/*
  Implementation of the pattern generation algorithm by Edelkamp. See:
  Stefan Edelkamp, Automated Creation of Pattern Database Search
  Heuristics. Proceedings of the 4th Workshop on Model Checking and
  Artificial Intelligence (MoChArt 2006), pp. 35-50, 2007.
*/
class PatternCollectionGeneratorGenetic : public PatternCollectionGenerator {
    // Maximum number of states for each pdb
    const int pdb_max_size;
    const int num_collections;
    const int num_episodes;
    const double mutation_probability;
    /* Specifies whether patterns in each pattern collection need to be disjoint
       or not. */
    const bool disjoint_patterns;
    std::shared_ptr<utils::RandomNumberGenerator> rng;

    std::shared_ptr<AbstractTask> task;

    // All current pattern collections.
    std::vector<std::vector<std::vector<bool>>> pattern_collections;

    // Store best pattern collection over all episodes and its fitness value.
    std::shared_ptr<PatternCollection> best_patterns;
    double best_fitness;

    /*
      The fitness values (from evaluate) are used as probabilities. Then
      num_collections many pattern collections are chosen from the vector of all
      pattern collections according to their probabilities. If all fitness
      values are 0, we select uniformly randomly. Note that the set of pattern
      collection where we select from is only changed by mutate, NOT by
      evaluate. All changes there (i.e. transformation and removal of irrelevant
      variables) are just temporary for improved PDB computation.
    */
    void select(const std::vector<double> &fitness_values);

    /*
      Iterate over all patterns and flip every variable (set 0 if 1 or 1 if 0)
      with the given probability from options. This method does not check for
      pdb_max_size or disjoint patterns.
    */
    void mutate();

    /*
      Transforms a bit vector (internal pattern representation in this class,
      mainly for easy mutation) to the "normal" pattern form vector<int>, which
      we need for ZeroOnePDBsHeuristic.
    */
    Pattern transform_to_pattern_normal_form(
        const std::vector<bool> &bitvector) const;

    /*
      Calculates the mean h-value (fitness value) for each pattern collection.
      For each pattern collection, we iterate over all patterns, first checking
      whether they respect the size limit, then modifying them in a way that
      only causally relevant variables remain in the patterns. Then the zero one
      partitioning pattern collection heuristic is constructed and its fitness
      ( = summed up mean h-values (dead ends are ignored) of all PDBs in the
      collection) computed. The overall best heuristic is eventually updated and
      saved for further episodes.
    */
    void evaluate(std::vector<double> &fitness_values);
    bool is_pattern_too_large(const Pattern &pattern) const;

    /*
      Mark used variables in variables_used and return true iff
      anything was already used (in which case we do not mark the
      remaining variables).
    */
    bool mark_used_variables(const Pattern &pattern,
                             std::vector<bool> &variables_used) const;
    void remove_irrelevant_variables(Pattern &pattern) const;

    /*
      Calculates the initial pattern collections with a next-fit bin packing
      algorithm. Variables are shuffled to obtain a different random ordering
      for every pattern collection. Then, variables are inserted into a pattern
      as long as pdb_max_sizes allows. If the "bin" is full, a new one is
      opened. This is repeated until all variables have been treated. Every
      initial pattern collection contains every variable exactly once and all
      initial patterns of each pattern collection are disjoint (regardless of
      the disjoint_patterns flag).
    */
    void bin_packing();

    /*
      Main genetic algorithm loop. All pattern collections are initialized with
      bin packing. In each iteration (or episode), all patterns are first
      mutated, then evaluated and finally some patterns in each collection are
      selected to be part of the next episode. Note that we do not do any kind
      of recombination.
    */
    void genetic_algorithm();

    virtual std::string name() const override;
    virtual PatternCollectionInformation compute_patterns(
        const std::shared_ptr<AbstractTask> &task) override;
public:
    PatternCollectionGeneratorGenetic(
        int pdb_max_size, int num_collections, int num_episodes,
        double mutation_probability, bool disjoint, int random_seed,
        utils::Verbosity verbosity);
};
}

#endif
