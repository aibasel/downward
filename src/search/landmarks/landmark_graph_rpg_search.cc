#include "landmark_graph_rpg_search.h"
#include "../option_parser.h"
#include "../plugin.h"

static LandmarkGraph *create(const std::vector<std::string> &config, int start,
                             int &end, bool dry_run);
static LandmarkGraphPlugin plugin("lm_search", create);

LandmarkGraphRpgSearch::LandmarkGraphRpgSearch(
    LandmarkGraph::Options &options, Exploration *exploration,
    bool uniform_sampling_, int max_depth_, int num_tries_)
    : LandmarkFactory(options, exploration),
      uniform_sampling(uniform_sampling_),
      max_depth(max_depth_),
      num_tries(num_tries_) {
}

LandmarkGraphRpgSearch::~LandmarkGraphRpgSearch() {
}


void LandmarkGraphRpgSearch::generate_landmarks() {
    cout << "Generating landmarks by search, verify using RPG method" << endl;

    vector<LandmarkNode *> not_landmarks;
    vector<int> evals;

    // insert goal landmarks and mark them as goals
    for (unsigned i = 0; i < g_goal.size(); i++) {
        LandmarkNode *lmp = &lm_graph->landmark_add_simple(g_goal[i]);
        lmp->in_goal = true;
    }
    // test all other possible facts
    for (int i = 0; i < g_variable_name.size(); i++) {
        for (int j = 0; j < g_variable_domain[i]; j++) {
            const pair<int, int> lm = make_pair(i, j);
            if (!lm_graph->simple_landmark_exists(lm)) {
                LandmarkNode *new_lm = &lm_graph->landmark_add_simple(lm);
                if ((*g_initial_state)[lm.first] != lm.second) {
                    int rp = relaxed_plan_length_without(new_lm);
                    //dump_node(new_lm);
                    //cout << "H = " << rp << endl;
                    if (rp >= 0) {
                        lm_graph->rm_landmark_node(new_lm);
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


void LandmarkGraphRpgSearch::landmark_search(LandmarkNode *node, int depth) {
    if (depth <= 0)
        return;
    bool lm_created = false;
    vector<LandmarkNode * > facts;
    vector<int> evals;
    for (int i = 0; i < g_variable_name.size() && !lm_created; i++) {
        for (int j = 0; j < g_variable_domain[i] && !lm_created; j++) {
            const pair<int, int> new_fact = make_pair(i, j);
            if (!lm_graph->landmark_exists(new_fact)) {
                set<pair<int, int> > lm;
                for (int n = 0; n < node->vars.size(); n++) {
                    lm.insert(make_pair(node->vars[n], node->vals[n]));
                }
                lm.insert(new_fact);
                LandmarkNode *new_lm = &lm_graph->landmark_add_disjunctive(lm);
                int rp = relaxed_plan_length_without(new_lm);
                //dump_node(new_lm);
                //cout << "H = " << rp << endl;
                if (rp >= 0) {
                    lm_graph->rm_landmark_node(new_lm);
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
int LandmarkGraphRpgSearch::choose_random(vector<int> &evals) {
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

int LandmarkGraphRpgSearch::relaxed_plan_length_without(LandmarkNode *exclude) {
    vector<pair<int, int> > exclude_props;
    hash_set<const Operator *, ex_hash_operator_ptr> exclude_ops;
    if (exclude != NULL) {
        for (int op = 0; op < g_operators.size(); op++) {
            if (achieves_non_conditional(g_operators[op], exclude))
                exclude_ops.insert(&g_operators[op]);
        }
        for (int i = 0; i < exclude->vars.size(); i++)
            exclude_props.push_back(make_pair(exclude->vars[i],
                                              exclude->vals[i]));
    }
    int val = lm_graph->get_exploration()->compute_ff_heuristic_with_excludes(
        *g_initial_state, exclude_props, exclude_ops);
    return val;
}

LandmarkGraph *create(const std::vector<string> &config, int start, int &end, bool dry_run) {
    LandmarkGraph::Options common_options;

    bool uniform_sampling = false;
    int max_depth = 10;
    int num_tries = 10;

    if (config.size() > start + 2 && config[start + 1] == "(") {
        end = start + 2;
        if (config[end] != ")") {
            NamedOptionParser option_parser;
            common_options.add_option_to_parser(option_parser);

            option_parser.add_int_option("max_depth",
                                         &max_depth,
                                         "max depth");

            option_parser.add_int_option("num_tries",
                                         &num_tries,
                                         "max number of tries");

            option_parser.add_bool_option("uniform_sampling",
                                          &uniform_sampling,
                                          "uniform sampling");

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
        LandmarkGraphRpgSearch lm_graph_factory(
            common_options, new Exploration(common_options.heuristic_options),
            uniform_sampling, max_depth, num_tries);
        LandmarkGraph *graph = lm_graph_factory.compute_lm_graph();
        return graph;
    }
}
