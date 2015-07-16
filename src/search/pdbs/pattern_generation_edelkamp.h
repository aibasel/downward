#ifndef PDBS_PATTERN_GENERATION_EDELKAMP_H
#define PDBS_PATTERN_GENERATION_EDELKAMP_H

#include "../operator_cost.h"
#include "../task_proxy.h"

#include <memory>
#include <vector>

class AbstractTask;
class Options;
class ZeroOnePDBsHeuristic;

/* Implementation of the pattern generation algorithm by Edelkamp. See:
   Stefan Edelkamp, Automated Creation of Pattern Database Search
   Heuristics. Proceedings of the 4th Workshop on Model Checking and
   Artificial Intelligence (MoChArt 2006), pp. 35-50, 2007. */

class PatternGenerationEdelkamp {
    const std::shared_ptr<AbstractTask> task;
    TaskProxy task_proxy;
    // Maximum number of states for each pdb
    const int pdb_max_size;
    const int num_collections;
    const int num_episodes;
    const double mutation_probability;
    /* Specifies whether patterns in each pattern collection need to be disjoint
       or not. */
    const bool disjoint_patterns;
    const OperatorCost cost_type;
    // All current pattern collections.
    std::vector<std::vector<std::vector<bool> > > pattern_collections;

    // Store the fitness value of the best pattern collection over all episodes.
    double best_fitness;
    /* Pointer to the heuristic in evaluate from the episode before.
       Used to free memory. */
    ZeroOnePDBsHeuristic *best_heuristic;

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
    void transform_to_pattern_normal_form(const std::vector<bool> &bitvector,
                                          std::vector<int> &pattern) const;

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
    bool is_pattern_too_large(const std::vector<int> &pattern) const;

    /*
      Mark used variables in variables_used and return true iff
      anything was already used (in which case we do not mark the
      remaining variables).
    */
    bool mark_used_variables(const std::vector<int> &pattern,
                             std::vector<bool> &variables_used) const;
    void remove_irrelevant_variables(std::vector<int> &pattern) const;

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
public:
    PatternGenerationEdelkamp(const Options &opts);
    virtual ~PatternGenerationEdelkamp();

    /*
      Returns the ZeroOnePDBsHeuristic created by PatternGenerationEdelkamp.
      Important: caller owns the returned pointer and has to take care of its
      deletion.
    */
    ZeroOnePDBsHeuristic *get_pattern_collection_heuristic() const {
        return best_heuristic;
    }
    void dump() const;
};

#endif
