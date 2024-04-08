#include "util.h"

#include "potential_function.h"
#include "potential_optimizer.h"

#include "../heuristic.h"

#include "../plugins/plugin.h"
#include "../task_utils/sampling.h"
#include "../utils/markup.h"

#include <limits>

using namespace std;

namespace potentials {
vector<State> sample_without_dead_end_detection(
    PotentialOptimizer &optimizer,
    int num_samples,
    utils::RandomNumberGenerator &rng) {
    const shared_ptr<AbstractTask> task = optimizer.get_task();
    const TaskProxy task_proxy(*task);
    State initial_state = task_proxy.get_initial_state();
    optimizer.optimize_for_state(initial_state);
    int init_h = optimizer.get_potential_function()->get_value(initial_state);
    sampling::RandomWalkSampler sampler(task_proxy, rng);
    vector<State> samples;
    samples.reserve(num_samples);
    for (int i = 0; i < num_samples; ++i) {
        samples.push_back(sampler.sample_state(init_h));
    }
    return samples;
}

string get_admissible_potentials_reference() {
    return "The algorithm is based on" + utils::format_conference_reference(
        {"Jendrik Seipp", "Florian Pommerening", "Malte Helmert"},
        "New Optimization Functions for Potential Heuristics",
        "https://ai.dmi.unibas.ch/papers/seipp-et-al-icaps2015.pdf",
        "Proceedings of the 25th International Conference on"
        " Automated Planning and Scheduling (ICAPS 2015)",
        "193-201",
        "AAAI Press",
        "2015");
}

void add_admissible_potentials_options_to_feature(
    plugins::Feature &feature, const string &description) {
    feature.document_language_support("action costs", "supported");
    feature.document_language_support("conditional effects", "not supported");
    feature.document_language_support("axioms", "not supported");
    feature.document_property("admissible", "yes");
    feature.document_property("consistent", "yes");
    feature.document_property("safe", "yes");
    feature.document_property("preferred operators", "no");
    feature.add_option<double>(
        "max_potential",
        "Bound potentials by this number. Using the bound {{{infinity}}} "
        "disables the bounds. In some domains this makes the computation of "
        "weights unbounded in which case no weights can be extracted. Using "
        "very high weights can cause numerical instability in the LP solver, "
        "while using very low weights limits the choice of potential "
        "heuristics. For details, see the ICAPS paper cited above.",
        "1e8",
        plugins::Bounds("0.0", "infinity"));
    lp::add_lp_solver_option_to_feature(feature);
    add_heuristic_options_to_feature(feature, description);
}


tuple<double, lp::LPSolverType, shared_ptr<AbstractTask>, bool, string,
      utils::Verbosity>
get_admissible_potential_arguments_from_options(
    const plugins::Options &opts) {
    return tuple_cat(
        make_tuple(opts.get<double>("max_potential")),
        lp::get_lp_solver_arguments_from_options(opts),
        get_heuristic_arguments_from_options(opts)
        );
}
}
