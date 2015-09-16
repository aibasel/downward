#ifndef PDBS_PATTERN_GENERATION_HASLUM_H
#define PDBS_PATTERN_GENERATION_HASLUM_H

#include "../operator_cost.h"
#include "../state_registry.h"
#include "../successor_generator.h"
#include "../task_proxy.h"

#include <map>
#include <memory>
#include <set>
#include <vector>

class CanonicalPDBsHeuristic;
class CountdownTimer;
class GlobalState;
class Options;
class PatternDatabase;

// Implementation of the pattern generation algorithm by Haslum et al.
class PatternGenerationHaslum {
    const std::shared_ptr<AbstractTask> task;
    TaskProxy task_proxy;
    // maximum number of states for each pdb
    const int pdb_max_size;
    // maximum added size of all pdbs
    const int collection_max_size;
    const int num_samples;
    // minimal improvement required for hill climbing to continue search
    const int min_improvement;
    const double max_time;
    const OperatorCost cost_type;
    CanonicalPDBsHeuristic *current_heuristic;
    SuccessorGenerator successor_generator;

    // for stats only
    int num_rejected;
    CountdownTimer *hill_climbing_timer;

    /*
      For the given pattern, all possible extensions of the pattern by one
      relevant variable are inserted into candidate_patterns. This may generate
      duplicated patterns.
    */
    void generate_candidate_patterns(
        const PatternDatabase *pdb,
        std::vector<std::vector<int> > &candidate_patterns);

    /*
      Generates the PatternDatabase for patterns in new_candidates if they have
      not been generated already.
    */
    std::size_t generate_pdbs_for_candidates(
        std::set<std::vector<int> > &generated_patterns,
        std::vector<std::vector<int> > &new_candidates,
        std::vector<PatternDatabase *> &candidate_pdbs) const;

    /*
      Performs num_samples random walks with a length (different for each
      random walk) chosen according to a binomial distribution with
      n = 4 * solution depth estimate and p = 0.5, starting from the initial
      state. In each step of a random walk, a random operator is taken and
      applied to the current state. If a dead end is reached or no more
      operators are applicable, the walk starts over again from the initial
      state. At the end of each random walk, the last state visited is taken as
      a sample state, thus totalling exactly num_samples of sample states.
    */
    void sample_states(std::vector<State> &samples, double average_operator_cost);

    /*
      Searches for the best improving pdb in candidate_pdbs according to the
      counting approximation and the given samples. Returns the improvement and
      the index of the best pdb in candidate_pdbs.
    */
    std::pair<int, int> find_best_improving_pdb(
        std::vector<State> &samples,
        std::vector<PatternDatabase *> &candidate_pdbs);

    /*
      Returns true iff the h-value of the new pattern (from pdb) plus the
      h-value of all maximal additive subsets from the current pattern
      collection heuristic if the new pattern was added to it is greater than
      the h-value of the current pattern collection.
    */
    bool is_heuristic_improved(
        PatternDatabase *pdb, const State &sample,
        const std::vector<std::vector<PatternDatabase *> > &max_additive_subsets);

    /*
      This is the core algorithm of this class. As soon as after an iteration,
      the improvement (according to the "counting approximation") is smaller
      than the minimal required improvement, the search is stopped. This method
      uses a vector to store PDBs to avoid re-computation of the same PDBs
      later. This is quite a large time gain, but may use too much memory. Also
      a set is used to store all patterns in their "normal form" for duplicate
      detection.
      TODO: This method computes all PDBs already for candidate iteration, but
      for each call of add_pattern for the current CanonicalPDBsHeuristic, only
      the pattern is passed as an argument and in CanonicalPDBsHeuristic, the
      PDB is *again* built. One could possibly avoid this by passing the PDB and
      adapt CanonicalPDBsHeuristic accordingly.
    */
    void hill_climbing(
        double average_operator_costs,
        std::vector<std::vector<int> > &initial_candidate_patterns);

    /*
      Initializes everything for the hill climbing algorithm. Note that the
      initial pattern collection (consisting of exactly one PDB for each goal
      variable) may break the maximum collection size limit, if the latter is
      set too small or if there are many goal variables with a large domain.
    */
    void initialize();
public:
    PatternGenerationHaslum(const Options &opts);
    virtual ~PatternGenerationHaslum();

    /*
      Returns the CanonicalPDBsHeuristic created by PatternGenerationHaslum.
      Important: caller owns the returned pointer and has to take care of its
      deletion.
    */
    CanonicalPDBsHeuristic *get_pattern_collection_heuristic() const {
        return current_heuristic;
    }
};

#endif
