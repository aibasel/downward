#ifndef PDBS_PATTERN_COLLECTION_GENERATOR_HILLCLIMBING_H
#define PDBS_PATTERN_COLLECTION_GENERATOR_HILLCLIMBING_H

#include "pattern_generator.h"
#include "types.h"

#include "../task_proxy.h"

#include <cstdlib>
#include <memory>
#include <set>
#include <vector>

namespace utils {
class CountdownTimer;
class RandomNumberGenerator;
}

namespace sampling {
class RandomWalkSampler;
}

namespace pdbs {
class CanonicalPDBsHeuristic;
class IncrementalCanonicalPDBs;
class PatternDatabase;

// Implementation of the pattern generation algorithm by Haslum et al.
class PatternCollectionGeneratorHillclimbing : public PatternCollectionGenerator {
    // maximum number of states for each pdb
    const int pdb_max_size;
    // maximum added size of all pdbs
    const int collection_max_size;
    const int num_samples;
    // minimal improvement required for hill climbing to continue search
    const int min_improvement;
    const double max_time;
    std::shared_ptr<utils::RandomNumberGenerator> rng;

    std::unique_ptr<IncrementalCanonicalPDBs> current_pdbs;

    // for stats only
    int num_rejected;
    utils::CountdownTimer *hill_climbing_timer;

    /*
      For the given PDB, all possible extensions of its pattern by one
      relevant variable are considered as candidate patterns. If the candidate
      pattern has not been previously considered (not contained in
      generated_patterns) and if building a PDB for it does not surpass the
      size limit, then the PDB is built and added to candidate_pdbs.

      The method returns the size of the largest PDB added to candidate_pdbs.
    */
    int generate_candidate_pdbs(
        const TaskProxy &task_proxy,
        const std::vector<std::vector<int>> &relevant_neighbours,
        const PatternDatabase &pdb,
        std::set<Pattern> &generated_patterns,
        PDBCollection &candidate_pdbs);

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
    void sample_states(
        const sampling::RandomWalkSampler &sampler,
        int init_h,
        std::vector<State> &samples);

    /*
      Searches for the best improving pdb in candidate_pdbs according to the
      counting approximation and the given samples. Returns the improvement and
      the index of the best pdb in candidate_pdbs.
    */
    std::pair<int, int> find_best_improving_pdb(
        const std::vector<State> &samples,
        const std::vector<int> &samples_h_values,
        PDBCollection &candidate_pdbs);

    /*
      Returns true iff the h-value of the new pattern (from pdb) plus the
      h-value of all pattern cliques from the current pattern
      collection heuristic if the new pattern was added to it is greater than
      the h-value of the current pattern collection.
    */
    bool is_heuristic_improved(
        const PatternDatabase &pdb,
        const State &sample,
        int h_collection,
        const PDBCollection &pdbs,
        const std::vector<PatternClique> &pattern_cliques);

    /*
      This is the core algorithm of this class. The initial PDB collection
      consists of one PDB for each goal variable. For each PDB of this initial
      collection, the set of candidate PDBs are added (see
      generate_candidate_pdbs) to the set of initial candidate PDBs.

      The main loop of the search computes a set of sample states (see
      sample_states) and uses this set to evaluate the set of all candidate PDBs
      (see find_best_improving_pdb, using the "counting approximation"). If the
      improvement obtained through adding the best PDB to the current heuristic
      is smaller than the minimal required improvement, the search is stopped.
      Otherwise, the best PDB is added to the heuristic and the candidate PDBs
      for this best PDB are computed (see generate_candidate_pdbs) and used for
      the next iteration.

      This method uses a set to store all patterns that are generated as
      candidate patterns in their "normal form" for duplicate detection.
      Futhermore, a vector stores the PDBs corresponding to the candidate
      patterns if its size does not surpass the user-specified size limit.
      Storing the PDBs has the only purpose to avoid re-computation of the same
      PDBs. This is quite a large time gain, but may use a lot of memory.
    */
    void hill_climbing(const TaskProxy &task_proxy);

    virtual std::string name() const override;

    /*
      Runs the hill climbing algorithm. Note that the
      initial pattern collection (consisting of exactly one PDB for each goal
      variable) may break the maximum collection size limit, if the latter is
      set too small or if there are many goal variables with a large domain.
    */
    virtual PatternCollectionInformation compute_patterns(
        const std::shared_ptr<AbstractTask> &task) override;
public:
    PatternCollectionGeneratorHillclimbing(
        int pdb_max_size, int collection_max_size, int num_samples,
        int min_improvement, double max_time, int random_seed,
        utils::Verbosity verbosity);
};

extern void add_hillclimbing_options_to_feature(
    plugins::Feature &feature);
std::tuple<int, int, int, int, double, int>
get_hillclimbing_arguments_from_options(
    const plugins::Options &opts);
}

#endif
