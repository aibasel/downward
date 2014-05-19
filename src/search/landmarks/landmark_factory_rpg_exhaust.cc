#include <vector>

#include "landmark_factory_rpg_exhaust.h"
#include "../state.h"
#include "../option_parser.h"
#include "../plugin.h"

/* Problem: We don't get any orders here. (All we have is the reasonable orders
   that are inferred later.) It's thus best to combine this landmark generation
   method with others, don't use it by itself. */

LandmarkFactoryRpgExhaust::LandmarkFactoryRpgExhaust(const Options &opts)
    : LandmarkFactory(opts) {
}

void LandmarkFactoryRpgExhaust::generate_landmarks() {
    cout << "Generating landmarks by testing all facts with RPG method" << endl;

    // insert goal landmarks and mark them as goals
    for (unsigned i = 0; i < g_goal.size(); i++) {
        LandmarkNode *lmp = &lm_graph->landmark_add_simple(g_goal[i]);
        lmp->in_goal = true;
    }
    // test all other possible facts
    const State &initial_state = g_initial_state();
    for (int i = 0; i < g_variable_name.size(); i++)
        for (int j = 0; j < g_variable_domain[i]; j++) {
            const pair<int, int> lm = make_pair(i, j);
            if (!lm_graph->simple_landmark_exists(lm)) {
                LandmarkNode *new_lm = &lm_graph->landmark_add_simple(lm);
                if (initial_state[lm.first] != lm.second && relaxed_task_solvable(true, new_lm)) {
                    assert(lm_graph->landmark_exists(lm));
                    LandmarkNode *node;
                    if (lm_graph->simple_landmark_exists(lm))
                        node = &lm_graph->get_simple_lm_node(lm);
                    else
                        node = &lm_graph->get_disj_lm_node(lm);
                    lm_graph->rm_landmark_node(node);
                }
            }
        }

}

static LandmarkGraph *_parse(OptionParser &parser) {
    parser.document_synopsis(
        "Exhaustive Landmarks",
        "Exhaustively checks for each fact if it is a landmark."
        "This check is done using relaxed planning.");
    parser.document_note(
        "Relevant options",
        "reasonable_orders, only_causal_landmarks");
    LandmarkGraph::add_options_to_parser(parser);

    Options opts = parser.parse();

    parser.document_language_support("conditional_effects",
                                     "ignored, i.e. not supported");
    opts.set<bool>("supports_conditional_effects", false);

    if (parser.dry_run()) {
        return 0;
    } else {
        opts.set<Exploration *>("explor", new Exploration(opts));
        LandmarkFactoryRpgExhaust lm_graph_factory(opts);
        LandmarkGraph *graph = lm_graph_factory.compute_lm_graph();
        return graph;
    }
}

static Plugin<LandmarkGraph> _plugin(
    "lm_exhaust", _parse);
