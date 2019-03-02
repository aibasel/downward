#include "landmark_factory_rpg_exhaust.h"

#include "landmark_graph.h"

#include "../option_parser.h"
#include "../plugin.h"
#include "../task_proxy.h"

#include <vector>
using namespace std;

namespace landmarks {
/* Problem: We don't get any orders here. (All we have is the reasonable orders
   that are inferred later.) It's thus best to combine this landmark generation
   method with others, don't use it by itself. */

LandmarkFactoryRpgExhaust::LandmarkFactoryRpgExhaust(const Options &opts)
    : LandmarkFactory(opts) {
}

void LandmarkFactoryRpgExhaust::generate_landmarks(
    const shared_ptr<AbstractTask> &task, Exploration &exploration) {
    TaskProxy task_proxy(*task);
    cout << "Generating landmarks by testing all facts with RPG method" << endl;

    // insert goal landmarks and mark them as goals
    for (FactProxy goal : task_proxy.get_goals()) {
        LandmarkNode &lmp = lm_graph->landmark_add_simple(goal.get_pair());
        lmp.in_goal = true;
    }
    // test all other possible facts
    State initial_state = task_proxy.get_initial_state();
    for (VariableProxy var : task_proxy.get_variables()) {
        for (int value = 0; value < var.get_domain_size(); ++value) {
            const FactPair lm(var.get_id(), value);
            if (!lm_graph->simple_landmark_exists(lm)) {
                vector<FactPair> facts = {lm};
                LandmarkNode node(facts, false);
                if (initial_state[lm.var].get_value() == lm.value ||
                    !relaxed_task_solvable(task_proxy, exploration, true, &node)) {
                    lm_graph->landmark_add_simple(lm);
                }
            }
        }
    }
}

bool LandmarkFactoryRpgExhaust::supports_conditional_effects() const {
    return false;
}

static shared_ptr<LandmarkFactory> _parse(OptionParser &parser) {
    parser.document_synopsis(
        "Exhaustive Landmarks",
        "Exhaustively checks for each fact if it is a landmark."
        "This check is done using relaxed planning.");
    parser.document_note(
        "Relevant options",
        "reasonable_orders, only_causal_landmarks");
    _add_options_to_parser(parser);

    Options opts = parser.parse();

    parser.document_language_support("conditional_effects",
                                     "ignored, i.e. not supported");

    if (parser.dry_run())
        return nullptr;
    else
        return make_shared<LandmarkFactoryRpgExhaust>(opts);
}

static Plugin<LandmarkFactory> _plugin("lm_exhaust", _parse);
}
