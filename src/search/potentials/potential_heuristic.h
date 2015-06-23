#ifndef POTENTIALS_POTENTIAL_HEURISTIC_H
#define POTENTIALS_POTENTIAL_HEURISTIC_H

#include "../global_state.h"
#include "../heuristic.h"
#include "../lp_solver_interface.h"
#include "../state_registry.h"

#include <vector>

namespace potentials {

enum OptimizationFunction {
    INITIAL_STATE,
    EACH_STATE,
    ALL_STATES_AVG,
    SAMPLES_AVG
};

class PotentialHeuristic: public Heuristic {
    friend class PotentialHeuristics;
    // TODO: avoid duplication from OCP
    class MatrixEntry {
    public:
        int row;
        int col;
        double element;
        MatrixEntry(int row_, int col_, double element_)
            : row(row_), col(col_), element(element_) {
        }
    };
#ifdef USE_LP
    OsiSolverInterface *lp_solver;
#endif
    OptimizationFunction optimization_function;
    Heuristic *sampling_heuristic;
    double sampling_steps_factor;
    int num_samples;
    double max_potential;
    double max_sampling_time;
    double max_filtering_time;
    bool debug;
    bool feasible_lp_solution;
    int num_cols;
    bool use_resolve;
    std::vector<std::vector<int> > fact_lp_vars;
    std::vector<std::vector<double> > fact_potential;
    int num_samples_covered;
    bool optimize_potential_for_state(const GlobalState &state);
    bool optimize_potential_for_all_states();
    bool optimize_potential_for_samples(const std::vector<GlobalState> &samples);
    void construct_lp();
    void sample_states(StateRegistry &sample_registry,
                       std::vector<GlobalState> &samples,
                       int num_samples,
                       Heuristic *heuristic);
    void filter_dead_ends(const std::vector<GlobalState> &samples,
                          std::vector<GlobalState> &non_dead_end_states);
#ifdef USE_LP
    void set_objective_for_state(const GlobalState &state);
    void set_objective_for_states(const std::vector<GlobalState> &states);
    void extract_lp_solution();
    bool solve_lp();
    void release_memory();
    void dump_potentials() const;
#endif
protected:
    virtual void initialize();
    virtual int compute_heuristic(const GlobalState &state);
public:
    explicit PotentialHeuristic(const Options &options);
    ~PotentialHeuristic();
};

void add_common_potential_options_to_parser(OptionParser &parser);

}
#endif
