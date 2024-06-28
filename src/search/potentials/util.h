#ifndef POTENTIALS_UTIL_H
#define POTENTIALS_UTIL_H

#include <memory>
#include <string>
#include <vector>
#include "../lp/lp_solver.h"
#include "../utils/logging.h"


class AbstractTask;
class State;

namespace plugins {
class Feature;
class Options;
}

namespace utils {
class RandomNumberGenerator;
}

namespace potentials {
class PotentialOptimizer;

std::vector<State> sample_without_dead_end_detection(
    PotentialOptimizer &optimizer,
    int num_samples,
    utils::RandomNumberGenerator &rng);

std::string get_admissible_potentials_reference();
void add_admissible_potentials_options_to_feature(
    plugins::Feature &feature, const std::string &description);
std::tuple<double, lp::LPSolverType, std::shared_ptr<AbstractTask>,
           bool, std::string, utils::Verbosity>
get_admissible_potential_arguments_from_options(
    const plugins::Options &opts);
}

#endif
