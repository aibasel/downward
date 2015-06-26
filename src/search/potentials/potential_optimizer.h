#ifndef POTENTIALS_POTENTIAL_OPTIMIZER_H
#define POTENTIALS_POTENTIAL_OPTIMIZER_H

#include "../lp_solver.h"

#include <memory>
#include <vector>

class GlobalState;
class Heuristic;
class Options;

namespace potentials {
class PotentialOptimizer {
    LPSolver lp_solver;
    const double max_potential;
    int num_facts;
    std::vector<std::vector<int> > fact_ids;
    std::vector<std::vector<double> > fact_potentials;

    void construct_lp();
    void set_lp_objective(const std::vector<double> &coefficients);
    bool solve_lp();
    void extract_lp_solution();

public:
    explicit PotentialOptimizer(const Options &opts);
    ~PotentialOptimizer() = default;

    bool potentials_are_bounded() const;

    bool optimize_for_state(const GlobalState &state);
    bool optimize_for_all_states();
    bool optimize_for_samples(const std::vector<GlobalState> &samples);

    bool has_optimal_solution() const;

    std::shared_ptr<Heuristic> get_heuristic() const;
};

void add_common_potential_options_to_parser(OptionParser &parser);

void optimize_for_samples(PotentialOptimizer &optimizer, int num_samples);
}
#endif
