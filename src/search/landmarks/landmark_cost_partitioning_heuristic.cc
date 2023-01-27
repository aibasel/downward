#include "landmark_cost_partitioning_heuristic.h"

#include "landmark_cost_assignment.h"
#include "landmark_factory.h"
#include "landmark_status_manager.h"

#include "../plugins/plugin.h"
#include "../task_utils/successor_generator.h"
#include "../task_utils/task_properties.h"
#include "../utils/markup.h"

#include <cmath>
#include <limits>

using namespace std;
using utils::ExitCode;

namespace landmarks {
LandmarkCostPartitioningHeuristic::LandmarkCostPartitioningHeuristic(
    const plugins::Options &opts)
    : LandmarkHeuristic(opts) {
    if (log.is_at_least_normal()) {
        log << "Initializing landmark cost partitioning heuristic..." << endl;
    }

    /*
      TODO: The landmark graph is already constructed by the base class for
       *LandmarkHeuristic*. The only reason why we get the landmark factory from
       the options again is to check whether it produces reasonable orderings.
       This could also be done by checking the landmark graph itself whether it
       has any reasonable orderings at all.

       In any case, doing this check only here means the landmark graph was
       already computed by the base class, which would not be necessary if we
       could ask the factory first, whether it computes reasonable orderings in
       the first place. Since this all happens in the constructor anyway and we
       abort if reasonable orderings are present, I don't think this is a big
       issue, though.
    */
    shared_ptr<LandmarkFactory> lm_graph_factory =
        opts.get<shared_ptr<LandmarkFactory>>("lm_factory");
    if (lm_graph_factory->computes_reasonable_orders()) {
        cerr << "Reasonable orderings should not be used for "
             << "admissible heuristics" << endl;
        utils::exit_with(ExitCode::SEARCH_INPUT_ERROR);
    } else if (task_properties::has_axioms(task_proxy)) {
        cerr << "cost partitioning does not support axioms" << endl;
        utils::exit_with(ExitCode::SEARCH_UNSUPPORTED);
    } else if (task_properties::has_conditional_effects(task_proxy) &&
               !conditional_effects_supported) {
        cerr << "conditional effects not supported by the landmark "
             << "generation method" << endl;
        utils::exit_with(ExitCode::SEARCH_UNSUPPORTED);
    }

    if (opts.get<bool>("optimal")) {
        lm_cost_assignment =
            utils::make_unique_ptr<LandmarkEfficientOptimalSharedCostAssignment>(
                task_properties::get_operator_costs(task_proxy),
                *lm_graph, opts.get<lp::LPSolverType>("lpsolver"));
    } else {
        lm_cost_assignment =
            utils::make_unique_ptr<LandmarkUniformSharedCostAssignment>(
                task_properties::get_operator_costs(task_proxy),
                *lm_graph, opts.get<bool>("alm"));
    }
}

int LandmarkCostPartitioningHeuristic::get_heuristic_value(
    const State &/*state*/) {
    double epsilon = 0.01;

    double h_val = lm_cost_assignment->cost_sharing_h_value(*lm_status_manager);
    if (h_val == numeric_limits<double>::max()) {
        return DEAD_END;
    } else {
        return static_cast<int>(ceil(h_val - epsilon));
    }
}

bool LandmarkCostPartitioningHeuristic::dead_ends_are_reliable() const {
    return true;
}

static shared_ptr<Heuristic> _parse(OptionParser &parser) {
    {
        parser.document_synopsis(
            "Landmark cost partitioning heuristic",
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

        LandmarkHeuristic::add_options_to_parser(parser);
        parser.add_option<bool>(
            "optimal",
            "use optimal (LP-based) cost sharing",
            "false");
        parser.add_option<bool>("alm", "use action landmarks", "true");
        lp::add_lp_solver_option_to_parser(parser);

        parser.document_note(
            "Optimal search",
            "You probably also want to add this heuristic as a lazy_evaluator "
            "in the A* algorithm to improve heuristic estimates.");
        parser.document_note(
            "Note",
            "To use ``optimal=true``, you must build the planner with LP "
            "support. See LPBuildInstructions.");

        parser.document_language_support(
            "action costs",
            "supported");
        parser.document_language_support(
            "conditional_effects",
            "not supported");
        parser.document_language_support(
            "axioms",
            "not allowed");

        parser.document_property(
            "admissible",
            "yes");
        /* TODO: This was "yes with admissible=true and optimal cost
            partitioning; otherwise no" before. Can we answer this now that
            the heuristic only cares about admissible? */
        parser.document_property(
            "consistent",
            "complicated; needs further thought");
        parser.document_property(
            "safe",
            "yes except on tasks with axioms or on tasks with "
            "conditional effects when using a LandmarkFactory "
            "not supporting them");
        parser.document_property(
            "preferred operators",
            "yes (if enabled; see ``pref`` option)");
    }
    Options opts = parser.parse();
    if (parser.dry_run())
        return nullptr;
    else
        return make_shared<LandmarkCostPartitioningHeuristic>(opts);
}

static Plugin<Evaluator> _plugin("lmcp", _parse);
}
