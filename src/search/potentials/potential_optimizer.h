#ifndef POTENTIALS_POTENTIAL_OPTIMIZER_H
#define POTENTIALS_POTENTIAL_OPTIMIZER_H

#include "../lp_solver.h"
#include "../task_proxy.h"

#include <memory>
#include <vector>

class Options;
class State;


namespace potentials {
class PotentialFunction;

/*
  Add admissible potential constraints to LP and allow optimizing for different
  objectives.
*/
class PotentialOptimizer {
    std::shared_ptr<AbstractTask> task;
    TaskProxy task_proxy;
    LPSolver lp_solver;
    const double max_potential;
    int num_lp_vars;
    std::vector<std::vector<int> > lp_var_ids;
    std::vector<std::vector<double> > fact_potentials;

    int get_lp_var_id(const FactProxy &fact) const;
    void initialize();
    void construct_lp();
    void set_lp_objective(const std::vector<double> &coefficients);
    bool solve_lp();
    bool has_optimal_solution() const;
    void extract_lp_solution();

public:
    explicit PotentialOptimizer(const Options &opts);
    ~PotentialOptimizer() = default;

    const TaskProxy &get_task_proxy() const;
    bool potentials_are_bounded() const;

    bool optimize_for_state(const State &state);
    bool optimize_for_all_states();
    bool optimize_for_samples(const std::vector<State> &samples);

    std::shared_ptr<PotentialFunction> get_potential_function() const;
};

void add_common_potentials_options_to_parser(OptionParser &parser);
}

#endif
