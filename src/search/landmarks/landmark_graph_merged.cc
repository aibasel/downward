#include "landmark_graph_merged.h"
#include "../option_parser.h"
#include "../plugin.h"

#include <set>

LandmarkGraphMerged::LandmarkGraphMerged(const Options &opts)
    : LandmarkFactory(opts),
      lm_graphs(opts.get_list<LandmarkGraph *>("lm_graphs")) {
}

LandmarkGraphMerged::~LandmarkGraphMerged() {
}

LandmarkNode *LandmarkGraphMerged::get_matching_landmark(const LandmarkNode &lm) const {
    if (!lm.disjunctive && !lm.conjunctive) {
        pair<int, int> lm_fact = make_pair(lm.vars[0], lm.vals[0]);
        if (lm_graph->simple_landmark_exists(lm_fact))
            return &lm_graph->get_simple_lm_node(lm_fact);
        else
            return 0;
    } else if (lm.disjunctive) {
        set<pair<int, int> > lm_facts;
        for (size_t i = 0; i < lm.vars.size(); ++i) {
            lm_facts.insert(make_pair(lm.vars[i], lm.vals[i]));
        }
        if (lm_graph->exact_same_disj_landmark_exists(lm_facts))
            return &lm_graph->get_disj_lm_node(make_pair(lm.vars[0], lm.vals[0]));
        else
            return 0;
    } else if (lm.conjunctive) {
        cerr << "Don't know how to handle conjunctive landmarks yet" << endl;
        exit_with(EXIT_UNSUPPORTED);
    }
    return 0;
}

void LandmarkGraphMerged::generate_landmarks() {
    cout << "Merging " << lm_graphs.size() << " landmark graphs" << endl;

    cout << "Adding simple landmarks" << endl;
    for (size_t i = 0; i < lm_graphs.size(); ++i) {
        const set<LandmarkNode *> &nodes = lm_graphs[i]->get_nodes();
        set<LandmarkNode *>::const_iterator it;
        for (it = nodes.begin(); it != nodes.end(); ++it) {
            const LandmarkNode &node = **it;
            pair<int, int> lm_fact = make_pair(node.vars[0], node.vals[0]);
            if (!node.conjunctive && !node.disjunctive && !lm_graph->landmark_exists(lm_fact)) {
                LandmarkNode &new_node = lm_graph->landmark_add_simple(lm_fact);
                new_node.in_goal = node.in_goal;
            }
        }
    }

    cout << "Adding disjunctive landmarks" << endl;
    for (size_t i = 0; i < lm_graphs.size(); ++i) {
        const set<LandmarkNode *> &nodes = lm_graphs[i]->get_nodes();
        set<LandmarkNode *>::const_iterator it;
        for (it = nodes.begin(); it != nodes.end(); ++it) {
            const LandmarkNode &node = **it;
            if (node.disjunctive) {
                set<pair<int, int> > lm_facts;
                bool exists = false;
                for (size_t j = 0; j < node.vars.size(); ++j) {
                    pair<int, int> lm_fact = make_pair(node.vars[j], node.vals[j]);
                    if (lm_graph->landmark_exists(lm_fact)) {
                        exists = true;
                        break;
                    }
                    lm_facts.insert(lm_fact);
                }
                if (!exists) {
                    LandmarkNode &new_node = lm_graph->landmark_add_disjunctive(lm_facts);
                    new_node.in_goal = node.in_goal;
                }
            } else if (node.conjunctive) {
                cerr << "Don't know how to handle conjunctive landmarks yet" << endl;
                exit_with(EXIT_UNSUPPORTED);
            }
        }
    }

    cout << "Adding orderings" << endl;
    for (size_t i = 0; i < lm_graphs.size(); ++i) {
        const set<LandmarkNode *> &nodes = lm_graphs[i]->get_nodes();
        for (const LandmarkNode *from_orig : nodes) {
            LandmarkNode *from = get_matching_landmark(*from_orig);
            if (from) {
                for (const auto &to : from_orig->children) {
                    const LandmarkNode &to_orig = *to.first;
                    edge_type e_type = to.second;
                    LandmarkNode *to_node = get_matching_landmark(to_orig);
                    if (to_node) {
                        edge_add(*from, *to_node, e_type);
                    } else {
                        cout << "Discarded to ordering" << endl;
                    }
                }
            } else {
                cout << "Discarded from ordering" << endl;
            }
        }
    }
}


static LandmarkGraph *_parse(OptionParser &parser) {
    parser.document_synopsis(
        "Merged Landmarks",
        "Merges the landmarks and orderings from the parameter landmarks");
    parser.document_note(
        "Precedence",
        "Fact landmarks take precedence over disjunctive landmarks, "
        "orderings take precedence in the usual manner "
        "(gn > nat > reas > o_reas). ");
    parser.document_note(
        "Relevant options",
        "Depends on landmarks");
    parser.document_note(
        "Note",
        "Does not currently support conjunctive landmarks");
    parser.add_list_option<LandmarkGraph *>("lm_graphs");
    LandmarkGraph::add_options_to_parser(parser);
    Options opts = parser.parse();

    opts.verify_list_non_empty<LandmarkGraph *>("lm_graphs");

    parser.document_language_support("conditional_effects",
                                     "supported if all components support them");

    if (parser.dry_run()) {
        return 0;
    } else {
        vector<LandmarkGraph *> lm_graphs = opts.get_list<LandmarkGraph *>("lm_graphs");
        bool supports_conditional_effects = true;
        for (size_t i = 0; i < lm_graphs.size(); ++i) {
            if (!lm_graphs[i]->supports_conditional_effects()) {
                supports_conditional_effects = false;
                break;
            }
        }
        opts.set<bool>("supports_conditional_effects", supports_conditional_effects);

        opts.set<Exploration *>("explor", new Exploration(opts));
        LandmarkGraphMerged lm_graph_factory(opts);
        LandmarkGraph *graph = lm_graph_factory.compute_lm_graph();
        return graph;
    }
}

static Plugin<LandmarkGraph> _plugin(
    "lm_merged", _parse);
