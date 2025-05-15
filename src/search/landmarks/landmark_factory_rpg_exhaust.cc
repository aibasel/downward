#include "landmark_factory_rpg_exhaust.h"

#include "exploration.h"
#include "landmark.h"
#include "landmark_graph.h"

#include "../task_proxy.h"

#include "../plugins/plugin.h"
#include "../utils/logging.h"

#include <vector>

using namespace std;

namespace landmarks {
/*
  Problem: We don't get any orderings here. (Reasonable orderings can also not
  be inferred later in the absence of natural orderings.) It's thus best to
  combine this landmark generation method with others, don't use it by itself.
*/

LandmarkFactoryRpgExhaust::LandmarkFactoryRpgExhaust(
    bool use_unary_relaxation, utils::Verbosity verbosity)
    : LandmarkFactoryRelaxation(verbosity),
      use_unary_relaxation(use_unary_relaxation) {
}

// Test whether the goal is reachable without achieving `landmark`.
static bool relaxed_task_solvable(
    const TaskProxy &task_proxy, Exploration &exploration,
    const Landmark &landmark, const bool use_unary_relaxation) {
    vector<vector<bool>> reached = exploration.compute_relaxed_reachability(
        landmark.atoms, use_unary_relaxation);

    for (FactProxy goal : task_proxy.get_goals()) {
        if (!reached[goal.get_variable().get_id()][goal.get_value()]) {
            return false;
        }
    }
    return true;
}

void LandmarkFactoryRpgExhaust::generate_goal_landmarks(
    const TaskProxy &task_proxy) const {
    for (FactProxy goal : task_proxy.get_goals()) {
        Landmark landmark({goal.get_pair()}, ATOMIC, true);
        landmark_graph->add_landmark(move(landmark));
    }
}

void LandmarkFactoryRpgExhaust::generate_all_atomic_landmarks(
    const TaskProxy &task_proxy, Exploration &exploration) const {
    for (VariableProxy var : task_proxy.get_variables()) {
        for (int value = 0; value < var.get_domain_size(); ++value) {
            const FactPair atom(var.get_id(), value);
            if (!landmark_graph->contains_atomic_landmark(atom)) {
                Landmark landmark({atom}, ATOMIC);
                if (!relaxed_task_solvable(task_proxy, exploration, landmark,
                                           use_unary_relaxation)) {
                    landmark_graph->add_landmark(move(landmark));
                }
            }
        }
    }
}

void LandmarkFactoryRpgExhaust::generate_relaxed_landmarks(
    const shared_ptr<AbstractTask> &task, Exploration &exploration) {
    TaskProxy task_proxy(*task);
    if (log.is_at_least_normal()) {
        log << "Generating landmarks by testing all atoms with RPG method"
            << endl;
    }
    generate_goal_landmarks(task_proxy);
    generate_all_atomic_landmarks(task_proxy, exploration);
}

bool LandmarkFactoryRpgExhaust::supports_conditional_effects() const {
    return false;
}

class LandmarkFactoryRpgExhaustFeature
    : public plugins::TypedFeature<LandmarkFactory, LandmarkFactoryRpgExhaust> {
public:
    LandmarkFactoryRpgExhaustFeature() : TypedFeature("lm_exhaust") {
        document_title("Exhaustive Landmarks");
        document_synopsis(
            "Exhaustively checks for each atom if it is a landmark."
            "This check is done using relaxed planning.");

        add_option<bool>(
            "use_unary_relaxation",
            "compute landmarks of the unary relaxation, i.e., landmarks "
            "for the delete relaxation of a task transformation such that each "
            "operator is split into one operator for each of its effects. This "
            "kind of landmarks was previously known as \"causal landmarks\". "
            "Setting the option to true can reduce the overall number of "
            "landmarks, which can make the search more memory-efficient but "
            "potentially less informative.",
            "false");
        add_landmark_factory_options_to_feature(*this);

        document_language_support(
            "conditional_effects",
            "ignored, i.e. not supported");
    }

    virtual shared_ptr<LandmarkFactoryRpgExhaust> create_component(
        const plugins::Options &opts) const override {
        return plugins::make_shared_from_arg_tuples<LandmarkFactoryRpgExhaust>(
            opts.get<bool>("use_unary_relaxation"),
            get_landmark_factory_arguments_from_options(opts));
    }
};

static plugins::FeaturePlugin<LandmarkFactoryRpgExhaustFeature> _plugin;
}
