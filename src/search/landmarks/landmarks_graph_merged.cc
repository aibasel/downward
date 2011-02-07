#include "landmarks_graph_merged.h"
#include "../option_parser.h"
#include "../plugin.h"

#include <set>

using namespace std;


LandmarksGraphMerged::LandmarksGraphMerged(const Options &opts)
    : LandmarksGraph(opts),
      lm_graphs(opts.get_list<LandmarksGraph *>("lm_graphs")) {
}

LandmarksGraphMerged::~LandmarksGraphMerged() {
}

LandmarkNode *LandmarksGraphMerged::get_matching_landmark(const LandmarkNode &lm) const {
    if (!lm.disjunctive && !lm.conjunctive) {
        pair<int, int> lm_fact = make_pair(lm.vars[0], lm.vals[0]);
        hash_map<pair<int, int>, LandmarkNode *, hash_int_pair>::const_iterator it =
            simple_lms_to_nodes.find(lm_fact);
        if (it != simple_lms_to_nodes.end()) {
            return it->second;
        } else {
            return 0;
        }
    } else if (lm.disjunctive) {
        set<pair<int, int> > lm_facts;
        for (int j = 0; j < lm.vars.size(); j++) {
            lm_facts.insert(make_pair(lm.vars[j], lm.vals[j]));
        }
        if (exact_same_disj_landmark_exists(lm_facts)) {
            hash_map<pair<int, int>, LandmarkNode *, hash_int_pair>::const_iterator it =
                disj_lms_to_nodes.find(make_pair(lm.vars[0], lm.vals[0]));
            return it->second;
        } else {
            return 0;
        }
    } else if (lm.conjunctive) {
        cerr << "Don't know how to handle conjunctive landmarks yet" << endl;
        abort();
    }
    return 0;
}

void LandmarksGraphMerged::generate_landmarks() {
    cout << "Merging " << lm_graphs.size() << " landmark graphs" << endl;

    cout << "Adding simple landmarks" << endl;
    for (int i = 0; i < lm_graphs.size(); i++) {
        const set<LandmarkNode *> &nodes = lm_graphs[i]->get_nodes();
        set<LandmarkNode *>::const_iterator it;
        for (it = nodes.begin(); it != nodes.end(); it++) {
            const LandmarkNode &node = **it;
            pair<int, int> lm_fact = make_pair(node.vars[0], node.vals[0]);
            if (!node.conjunctive && !node.disjunctive && !landmark_exists(lm_fact)) {
                LandmarkNode &new_node = landmark_add_simple(lm_fact);
                new_node.in_goal = node.in_goal;
            }
        }
    }

    cout << "Adding disjunctive landmarks" << endl;
    for (int i = 0; i < lm_graphs.size(); i++) {
        const set<LandmarkNode *> &nodes = lm_graphs[i]->get_nodes();
        set<LandmarkNode *>::const_iterator it;
        for (it = nodes.begin(); it != nodes.end(); it++) {
            const LandmarkNode &node = **it;
            if (node.disjunctive) {
                set<pair<int, int> > lm_facts;
                for (int j = 0; j < node.vars.size(); j++) {
                    lm_facts.insert(make_pair(node.vars[j], node.vals[j]));
                }
                if (!disj_landmark_exists(lm_facts)) {
                    LandmarkNode &new_node = landmark_add_disjunctive(lm_facts);
                    new_node.in_goal = node.in_goal;
                }
            } else if (node.conjunctive) {
                cerr << "Don't know how to handle conjunctive landmarks yet" << endl;
                abort();
            }
        }
    }

    cout << "Adding orderings" << endl;
    for (int i = 0; i < lm_graphs.size(); i++) {
        const set<LandmarkNode *> &nodes = lm_graphs[i]->get_nodes();
        set<LandmarkNode *>::const_iterator it;
        for (it = nodes.begin(); it != nodes.end(); it++) {
            const LandmarkNode &from_orig = **it;
            LandmarkNode *from = get_matching_landmark(from_orig);
            if (from) {
                hash_map<LandmarkNode *, edge_type, hash_pointer>::const_iterator to_it;
                for (to_it = from_orig.children.begin(); to_it != from_orig.children.end(); to_it++) {
                    const LandmarkNode &to_orig = *to_it->first;
                    edge_type e_type = to_it->second;
                    LandmarkNode *to = get_matching_landmark(to_orig);
                    if (to) {
                        edge_add(*from, *to, e_type);
                    } else {
                        cout << "No to" << endl;
                    }
                }
            } else {
                cout << "No from" << endl;
            }
        }
    }
}


static LandmarksGraph *_parse(OptionParser &parser) {
    parser.add_list_option<LandmarksGraph *>("lm_graphs");
    LandmarksGraph::add_options_to_parser(parser);
    Options opts = parser.parse();
    if(parser.help_mode())
        return 0;
    
    if (opts.get_list<LandmarksGraph *>("lm_graphs").empty()) {
        parser.error("list of landmarks graph must not be empty");
    }

    if (parser.dry_run()) {
        return 0;
    } else {
        opts.set<Exploration *>("explor", new Exploration(opts));
        LandmarksGraph *graph = new LandmarksGraphMerged(opts);
        LandmarksGraph::build_lm_graph(graph);
        return graph;
    }
}

static LandmarkGraphPlugin landmarks_graph_merged_plugin(
    "lm_merged", _parse);
