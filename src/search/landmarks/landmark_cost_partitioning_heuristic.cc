#include "landmark_cost_partitioning_heuristic.h"

#include "landmark_cost_partitioning_algorithms.h"
#include "landmark_factory.h"
#include "landmark_status_manager.h"

#include "../plugins/plugin.h"
#include "../task_utils/successor_generator.h"
#include "../task_utils/task_properties.h"
#include "../utils/markup.h"

#include <cmath>
#include <limits>

using namespace std;

namespace landmarks {
LandmarkCostPartitioningHeuristic::LandmarkCostPartitioningHeuristic(
    const plugins::Options &opts)
    : LandmarkHeuristic(opts) {
    if (log.is_at_least_normal()) {
        log << "Initializing landmark cost partitioning heuristic..." << endl;
    }
    check_unsupported_features(opts);
    initialize(opts);
    set_cost_partitioning_algorithm(opts);
}

void LandmarkCostPartitioningHeuristic::check_unsupported_features(
    const plugins::Options &opts) {
    shared_ptr<LandmarkFactory> lm_graph_factory =
        opts.get<shared_ptr<LandmarkFactory>>("lm_factory");

    if (task_properties::has_axioms(task_proxy)) {
        cerr << "Cost partitioning does not support axioms." << endl;
        utils::exit_with(utils::ExitCode::SEARCH_UNSUPPORTED);
    }

    if (!lm_graph_factory->supports_conditional_effects()
        && task_properties::has_conditional_effects(task_proxy)) {
        cerr << "Conditional effects not supported by the landmark "
             << "generation method." << endl;
        utils::exit_with(utils::ExitCode::SEARCH_UNSUPPORTED);
    }
}

void LandmarkCostPartitioningHeuristic::set_cost_partitioning_algorithm(
    const plugins::Options &opts) {
    auto method = opts.get<CostPartitioningMethod>("cost_partitioning");
    if (method == CostPartitioningMethod::OPTIMAL) {
        cost_partitioning_algorithm =
            utils::make_unique_ptr<OptimalCostPartitioningAlgorithm>(
                task_properties::get_operator_costs(task_proxy),
                *lm_graph, opts.get<lp::LPSolverType>("lpsolver"));
    } else if (method == CostPartitioningMethod::UNIFORM) {
        cost_partitioning_algorithm =
            utils::make_unique_ptr<UniformCostPartitioningAlgorithm>(
                task_properties::get_operator_costs(task_proxy),
                *lm_graph, opts.get<bool>("alm"));
    } else {
        ABORT("Unknown cost partitioning method");
    }
}

int LandmarkCostPartitioningHeuristic::get_heuristic_value(
    const State &ancestor_state) {
    double epsilon = 0.01;

    double h_val =
        cost_partitioning_algorithm->get_cost_partitioned_heuristic_value(
            *lm_status_manager, ancestor_state);
    if (h_val == numeric_limits<double>::max()) {
        return DEAD_END;
    } else {
        return static_cast<int>(ceil(h_val - epsilon));
    }
}

bool LandmarkCostPartitioningHeuristic::dead_ends_are_reliable() const {
    return true;
}

class LandmarkCostPartitioningHeuristicFeature : public plugins::TypedFeature<Evaluator, LandmarkCostPartitioningHeuristic> {
public:
    LandmarkCostPartitioningHeuristicFeature() : TypedFeature("landmark_cost_partitioning") {
        document_title("Landmark cost partitioning heuristic");
        document_synopsis(
            "Formerly known as the admissible landmark heuristic.\n"
            "See the papers" +
            utils::format_conference_reference(
                {"Erez Karpas", "Carmel Domshlak"},
                "Cost-Optimal Planning with Landmarks",
                "https://www.ijcai.org/Proceedings/09/Papers/288.pdf",
                "Proceedings of the 21st International Joint Conference on "
                "Artificial Intelligence (IJCAI 2009)",
                "1728-1733",
                "AAAI Press",
                "2009") +
            "and" +
            utils::format_conference_reference(
                {"Emil Keyder and Silvia Richter and Malte Helmert"},
                "Sound and Complete Landmarks for And/Or Graphs",
                "https://ai.dmi.unibas.ch/papers/keyder-et-al-ecai2010.pdf",
                "Proceedings of the 19th European Conference on Artificial "
                "Intelligence (ECAI 2010)",
                "335-340",
                "IOS Press",
                "2010"));

        LandmarkHeuristic::add_options_to_feature(*this);
        add_option<CostPartitioningMethod>(
            "cost_partitioning",
            "strategy for partitioning operator costs among landmarks",
            "uniform");
        add_option<bool>("alm", "use action landmarks", "true");
        lp::add_lp_solver_option_to_feature(*this);

        document_note(
            "Usage with A*",
            "We recommend to add this heuristic as lazy_evaluator when using "
            "it in the A* algorithm. This way, the heuristic is recomputed "
            "before a state is expanded, leading to improved estimates that "
            "incorporate all knowledge gained from paths that were found after "
            "the state was inserted into the open list.");
        document_note(
            "Consistency",
            "The heuristic is consistent along single paths if it is "
            "set as lazy_evaluator; i.e. when expanding s then we have "
            "h(s) <= h(s')+cost(a) for all successors s' of s reached "
            "with a. But newly found paths to s can increase h(s), at "
            "which point the above inequality might not hold anymore.");
        document_note(
            "Optimal Cost Partitioning",
            "To use ``cost_partitioning=optimal``, you must build the planner with LP "
            "support. See [build instructions https://github.com/aibasel/downward/blob/main/BUILD.md].");
        document_note(
            "Preferred operators",
            "Preferred operators should not be used for optimal planning. "
            "See Evaluator#Landmark_sum_heuristic for more information "
            "on using preferred operators; the comments there also apply "
            "to this heuristic.");

        document_language_support("action costs", "supported");
        document_language_support(
            "conditional_effects",
            "supported if the LandmarkFactory supports them; otherwise "
            "not supported");
        document_language_support("axioms", "not allowed");

        document_property("admissible", "yes");
        document_property("consistent",
                          "no; see document note about consistency");
        document_property("safe", "yes");
    }
};

static plugins::FeaturePlugin<LandmarkCostPartitioningHeuristicFeature> _plugin;

static plugins::TypedEnumPlugin<CostPartitioningMethod> _enum_plugin({
        {"optimal",
         "use optimal (LP-based) cost partitioning"},
        {"uniform",
         "partition operator costs uniformly among all landmarks "
         "achieved by that operator"},
    });
}
