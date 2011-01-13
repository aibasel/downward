#include <vector>
#include <ext/hash_map>

#include "landmarks_graph_rpg_exhaust.h"
#include "../state.h"
#include "../option_parser.h"
#include "../plugin.h"

static LandmarkGraphPlugin landmarks_graph_exhaust_plugin(
    "lm_exhaust", LandmarksGraphExhaust::create);

/* Problem: We don't get any orders here. (All we have is the reasonable orders
   that are inferred later.) It's thus best to combine this landmark generation
   method with others, don't use it by itself. */

LandmarksGraphExhaust::LandmarksGraphExhaust(LandmarksGraph::LandmarkGraphOptions &options, Exploration *exploration)
: lm_graph(new LandmarksGraph(options, exploration)) {
    lm_graph->read_external_inconsistencies();
    generate_landmarks();
    LandmarksGraph::build_lm_graph(lm_graph);
}

void LandmarksGraphExhaust::generate_landmarks() {
    cout << "Generating landmarks by testing all facts with RPG method" << endl;

    // insert goal landmarks and mark them as goals
    for (unsigned i = 0; i < g_goal.size(); i++) {
        LandmarkNode *lmp = &lm_graph->landmark_add_simple(g_goal[i]);
        lmp->in_goal = true;
    }
    // test all other possible facts
    for (int i = 0; i < g_variable_name.size(); i++)
        for (int j = 0; j < g_variable_domain[i]; j++) {
            const pair<int, int> lm = make_pair(i, j);
            if (!lm_graph->simple_landmark_exists(lm)) {
                LandmarkNode *new_lm = &lm_graph->landmark_add_simple(lm);
                if ((*g_initial_state)[lm.first] != lm.second && lm_graph->relaxed_task_solvable(true, new_lm))
                    lm_graph->rm_landmark(lm);
            }
        }

}

LandmarksGraph *LandmarksGraphExhaust::get_lm_graph() {
    return lm_graph;
}

LandmarksGraph *LandmarksGraphExhaust::create(
    const std::vector<string> &config, int start, int &end, bool dry_run) {
    LandmarksGraph::LandmarkGraphOptions common_options;

    if (config.size() > start + 2 && config[start + 1] == "(") {
        end = start + 2;
        if (config[end] != ")") {
            NamedOptionParser option_parser;
            common_options.add_option_to_parser(option_parser);

            option_parser.parse_options(config, end, end, dry_run);
            end++;
        }
        if (config[end] != ")")
            throw ParseError(end);
    } else {
        end = start;
    }

    if (dry_run) {
        return 0;
    } else {
        /*LandmarksGraph *graph = new LandmarksGraphExhaust(common_options,
                                                          new Exploration);
        LandmarksGraph::build_lm_graph(graph);
        return graph;*/
        LandmarksGraphExhaust lm_graph_factory(common_options, new Exploration);
        LandmarksGraph *graph = lm_graph_factory.get_lm_graph();
        return graph;
    }
}
