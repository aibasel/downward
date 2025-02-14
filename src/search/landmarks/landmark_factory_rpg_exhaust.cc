#include "landmark_factory_rpg_exhaust.h"

#include "landmark.h"
#include "landmark_graph.h"

#include "../task_proxy.h"

#include "../plugins/plugin.h"
#include "../utils/logging.h"

#include <vector>
using namespace std;

namespace landmarks {
/* Problem: We don't get any orders here. (All we have is the reasonable orders
   that are inferred later.) It's thus best to combine this landmark generation
   method with others, don't use it by itself. */

LandmarkFactoryRpgExhaust::LandmarkFactoryRpgExhaust(
    bool use_unary_relaxation, utils::Verbosity verbosity)
    : LandmarkFactoryRelaxation(verbosity),
      use_unary_relaxation(use_unary_relaxation) {
}

void LandmarkFactoryRpgExhaust::generate_relaxed_landmarks(
    const shared_ptr<AbstractTask> &task, Exploration &exploration) {
    TaskProxy task_proxy(*task);
    if (log.is_at_least_normal()) {
        log << "Generating landmarks by testing all facts with RPG method" << endl;
    }

    // insert goal landmarks and mark them as goals
    for (FactProxy goal : task_proxy.get_goals()) {
        Landmark landmark({goal.get_pair()}, false, false, true);
        lm_graph->add_landmark(move(landmark));
    }
    // test all other possible facts
    for (VariableProxy var : task_proxy.get_variables()) {
        for (int value = 0; value < var.get_domain_size(); ++value) {
            const FactPair lm(var.get_id(), value);
            if (!lm_graph->contains_simple_landmark(lm)) {
                Landmark landmark({lm}, false, false);
                if (!relaxed_task_solvable(task_proxy, exploration, landmark,
                                           use_unary_relaxation)) {
                    lm_graph->add_landmark(move(landmark));
                }
            }
        }
    }
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
            "Exhaustively checks for each fact if it is a landmark."
            "This check is done using relaxed planning.");

        add_option<bool>(
            "use_unary_relaxation",
            "compute landmarks of the unary relaxation, i.e., landmarks "
            "for the delete relaxation of a task transformation such that each "
            "operator is split into one operator for each of its effects. This "
            "kind of landmark was previously known as \"causal landmarks\". "
            "Setting the option to true can reduce the overall number of "
            "landmarks, which can make the search more memory-efficient but "
            "potentially less informative.",
            "false");
        add_landmark_factory_options_to_feature(*this);

        document_language_support(
            "conditional_effects",
            "ignored, i.e. not supported");
    }

    virtual shared_ptr<LandmarkFactoryRpgExhaust>
    create_component(const plugins::Options &opts) const override {
        return plugins::make_shared_from_arg_tuples<LandmarkFactoryRpgExhaust>(
            opts.get<bool>("use_unary_relaxation"),
            get_landmark_factory_arguments_from_options(opts));
    }
};

static plugins::FeaturePlugin<LandmarkFactoryRpgExhaustFeature> _plugin;
}
