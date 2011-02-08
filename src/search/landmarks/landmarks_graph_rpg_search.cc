#include "landmarks_graph_rpg_search.h"
#include "../option_parser.h"
#include "../plugin.h"


LandmarksGraphRpgSearch::LandmarksGraphRpgSearch(const Options &opts)
    : LandmarksGraph(opts),
      uniform_sampling(opts.get<bool>("uniform_sampling")),
      max_depth(opts.get<int>("max_depth")),
      num_tries(opts.get<int>("num_tries")) {
}

LandmarksGraphRpgSearch::~LandmarksGraphRpgSearch() {
}


void LandmarksGraphRpgSearch::generate_landmarks() {
    cout << "Generating landmarks by search, verify using RPG method" << endl;

    vector<LandmarkNode *> not_landmarks;
    vector<int> evals;

    // insert goal landmarks and mark them as goals
    for (unsigned i = 0; i < g_goal.size(); i++) {
        LandmarkNode *lmp = &landmark_add_simple(g_goal[i]);
        lmp->in_goal = true;
    }
    // test all other possible facts
    for (int i = 0; i < g_variable_name.size(); i++) {
        for (int j = 0; j < g_variable_domain[i]; j++) {
            const pair<int, int> lm = make_pair(i, j);
            if (!simple_landmark_exists(lm)) {
                LandmarkNode *new_lm = &(landmark_add_simple(lm));
                if ((*g_initial_state)[lm.first] != lm.second) {
                    int rp = relaxed_plan_length_without(new_lm);
                    //dump_node(new_lm);
                    //cout << "H = " << rp << endl;
                    if (rp >= 0) {
                        rm_landmark_node(new_lm);
                        not_landmarks.push_back(new_lm);
                        evals.push_back(rp);
                    }
                }
            }
        }
    }
    for (int i = 0; i < not_landmarks.size(); i++) {
        for (int j = 0; j < num_tries; j++) {
            landmark_search(not_landmarks[i], max_depth);
        }
    }
}


void LandmarksGraphRpgSearch::landmark_search(LandmarkNode *node, int depth) {
    if (depth <= 0)
        return;
    bool lm_created = false;
    vector<LandmarkNode * > facts;
    vector<int> evals;
    for (int i = 0; i < g_variable_name.size() && !lm_created; i++) {
        for (int j = 0; j < g_variable_domain[i] && !lm_created; j++) {
            const pair<int, int> new_fact = make_pair(i, j);
            if (!landmark_exists(new_fact)) {
                set<pair<int, int> > lm;
                for (int n = 0; n < node->vars.size(); n++) {
                    lm.insert(make_pair(node->vars[n], node->vals[n]));
                }
                lm.insert(new_fact);
                LandmarkNode *new_lm = &landmark_add_disjunctive(lm);
                int rp = relaxed_plan_length_without(new_lm);
                //dump_node(new_lm);
                //cout << "H = " << rp << endl;
                if (rp >= 0) {
                    rm_landmark_node(new_lm);
                    facts.push_back(new_lm);
                    evals.push_back(rp);
                } else {
                    lm_created = true;
                    break;
                }
            }
        }
    }
    if (!lm_created) {
        int next_index = choose_random(evals);
        landmark_search(facts[next_index], depth - 1);
    }
}

/*!
 * Return a random index between 0 and evals.size() - 1, with probability for i being evals[i]
 */
int LandmarksGraphRpgSearch::choose_random(vector<int> &evals) {
    int ret = 0;
    if (uniform_sampling) {
        ret = rand() % evals.size();
    } else {
        int sum = 0;
        for (int i = 0; i < evals.size(); i++) {
            sum = sum + evals[i];
        }
        int val = rand() % sum;
        ret = -1;
        while (val >= 0) {
            ret++;
            val = val - evals[ret];
        }
    }
    return ret;
}


static LandmarksGraph *_parse(OptionParser &parser) {
    parser.add_option<int>("max_depth", 10, "max depth");
    parser.add_option<int>("num_tries", 10, "max number of tries");
    parser.add_option<bool>("uniform_sampling", false, "uniform sampling");
    LandmarksGraph::add_options_to_parser(parser);
    Options opts = parser.parse();
    if (parser.help_mode())
        return 0;

    if (parser.dry_run()) {
        return 0;
    } else {
        opts.set<Exploration *>("explor", new Exploration(opts));
        LandmarksGraph *graph = new LandmarksGraphRpgSearch(opts);
        LandmarksGraph::build_lm_graph(graph);
        return graph;
    }
}

static LandmarkGraphPlugin _plugin(
    "lm_search", _parse);
