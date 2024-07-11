#include "potential_function.h"
#include "potential_max_heuristic.h"
#include "potential_optimizer.h"
#include "util.h"

#include "../plugins/plugin.h"
#include "../utils/rng.h"
#include "../utils/rng_options.h"

#include <memory>
#include <vector>

using namespace std;

namespace potentials {
static void filter_dead_ends(PotentialOptimizer &optimizer, vector<State> &samples) {
    assert(!optimizer.potentials_are_bounded());
    vector<State> non_dead_end_samples;
    for (const State &sample : samples) {
        optimizer.optimize_for_state(sample);
        if (optimizer.has_optimal_solution())
            non_dead_end_samples.push_back(sample);
    }
    swap(samples, non_dead_end_samples);
}

static void optimize_for_samples(
    PotentialOptimizer &optimizer,
    int num_samples,
    utils::RandomNumberGenerator &rng) {
    vector<State> samples = sample_without_dead_end_detection(
        optimizer, num_samples, rng);
    if (!optimizer.potentials_are_bounded()) {
        filter_dead_ends(optimizer, samples);
    }
    optimizer.optimize_for_samples(samples);
}

/*
  Compute multiple potential functions that are optimized for different
  sets of samples.
*/
static vector<unique_ptr<PotentialFunction>>
create_sample_based_potential_functions(
    int num_samples, int num_heuristics, double max_potential,
    lp::LPSolverType lpsolver,
    const shared_ptr<AbstractTask> &transform, int random_seed) {
    vector<unique_ptr<PotentialFunction>> functions;
    PotentialOptimizer optimizer(transform, lpsolver, max_potential);
    shared_ptr<utils::RandomNumberGenerator> rng(
        utils::get_rng(random_seed));
    for (int i = 0; i < num_heuristics; ++i) {
        optimize_for_samples(optimizer, num_samples, *rng);
        functions.push_back(optimizer.get_potential_function());
    }
    return functions;
}

class SampleBasedPotentialMaxHeuristicFeature
    : public plugins::TypedFeature<Evaluator, PotentialMaxHeuristic> {
public:
    SampleBasedPotentialMaxHeuristicFeature() : TypedFeature("sample_based_potentials") {
        document_subcategory("heuristics_potentials");
        document_title("Sample-based potential heuristics");
        document_synopsis(
            "Maximum over multiple potential heuristics optimized for samples. " +
            get_admissible_potentials_reference());
        add_option<int>(
            "num_heuristics",
            "number of potential heuristics",
            "1",
            plugins::Bounds("0", "infinity"));
        add_option<int>(
            "num_samples",
            "Number of states to sample",
            "1000",
            plugins::Bounds("0", "infinity"));
        add_admissible_potentials_options_to_feature(
            *this, "sample_based_potentials");
        utils::add_rng_options_to_feature(*this);
    }

    virtual shared_ptr<PotentialMaxHeuristic> create_component(
        const plugins::Options &opts,
        const utils::Context &) const override {
        return make_shared<PotentialMaxHeuristic>(
            create_sample_based_potential_functions(
                opts.get<int>("num_samples"),
                opts.get<int>("num_heuristics"),
                opts.get<double>("max_potential"),
                opts.get<lp::LPSolverType>("lpsolver"),
                opts.get<shared_ptr<AbstractTask>>("transform"),
                opts.get<int>("random_seed")
                ),
            opts.get<shared_ptr<AbstractTask>>("transform"),
            opts.get<bool>("cache_estimates"),
            opts.get<string>("description"),
            opts.get<utils::Verbosity>("verbosity")
            );
    }
};

static plugins::FeaturePlugin<SampleBasedPotentialMaxHeuristicFeature> _plugin;
}
