#ifndef POTENTIALS_POTENTIAL_OPTIMIZER_H
#define POTENTIALS_POTENTIAL_OPTIMIZER_H

#include "../lp_solver.h"

#include <memory>
#include <vector>

class GlobalState;
class Heuristic;
class Options;
class StateRegistry;

namespace potentials {

enum OptimizationFunction {
    INITIAL_STATE,
    EACH_STATE,
    ALL_STATES_AVG,
    SAMPLES_AVG
};

class PotentialOptimizer {
    LPSolver *lp_solver;
    OptimizationFunction optimization_function;
    Heuristic *sampling_heuristic;
    double sampling_steps_factor;
    int num_samples;
    double max_potential;
    double max_sampling_time;
    double max_filtering_time;
    bool debug;
    int num_cols;
    std::vector<std::vector<int> > lp_var_ids;
    std::vector<std::vector<double> > fact_potentials;
    int num_samples_covered;
    bool optimize_potential_for_state(const GlobalState &state);
    bool optimize_potential_for_all_states();
    bool optimize_potential_for_samples(const std::vector<GlobalState> &samples);
    void construct_lp();
    void sample_states(StateRegistry &sample_registry,
                       std::vector<GlobalState> &samples,
                       int num_samples,
                       Heuristic &heuristic);
    void filter_dead_ends(const std::vector<GlobalState> &samples,
                          std::vector<GlobalState> &non_dead_end_states);
    void set_objective_for_state(const GlobalState &state);
    void set_objective_for_states(const std::vector<GlobalState> &states);
    void extract_lp_solution();
    bool solve_lp();
    void release_memory();
    void dump_potentials() const;

public:
    explicit PotentialOptimizer(const Options &options);
    ~PotentialOptimizer() = default;

    bool has_optimal_solution() const;
    const std::vector<std::vector<double> > &get_fact_potentials() const;
};

void add_common_potential_options_to_parser(OptionParser &parser);

}
#endif
