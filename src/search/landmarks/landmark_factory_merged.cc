#include "landmark_factory_merged.h"

#include "landmark_graph.h"

#include "../option_parser.h"
#include "../plugin.h"

#include "../utils/system.h"

#include <set>

using namespace std;
using utils::ExitCode;

namespace landmarks {
class LandmarkNode;

LandmarkFactoryMerged::LandmarkFactoryMerged(const Options &opts)
    : LandmarkFactory(opts),
      lm_factories(opts.get_list<LandmarkFactory *>("lm_factories")) {
}

LandmarkNode *LandmarkFactoryMerged::get_matching_landmark(const LandmarkNode &lm) const {
    if (!lm.disjunctive && !lm.conjunctive) {
        const FactPair &lm_fact = lm.facts[0];
        if (lm_graph->simple_landmark_exists(lm_fact))
            return &lm_graph->get_simple_lm_node(lm_fact);
        else
            return 0;
    } else if (lm.disjunctive) {
        set<FactPair> lm_facts(lm.facts.begin(), lm.facts.end());
        if (lm_graph->exact_same_disj_landmark_exists(lm_facts))
            return &lm_graph->get_disj_lm_node(lm.facts[0]);
        else
            return 0;
    } else if (lm.conjunctive) {
        cerr << "Don't know how to handle conjunctive landmarks yet" << endl;
        utils::exit_with(ExitCode::UNSUPPORTED);
    }
    return 0;
}

void LandmarkFactoryMerged::generate_landmarks(
    const shared_ptr<AbstractTask> &task, Exploration &exploration) {
    cout << "Merging " << lm_factories.size() << " landmark graphs" << endl;

    for (LandmarkFactory *lm_factory : lm_factories) {
        lm_graphs.push_back(lm_factory->compute_lm_graph(task, exploration));
    }

    cout << "Adding simple landmarks" << endl;
    for (size_t i = 0; i < lm_graphs.size(); ++i) {
        const set<LandmarkNode *> &nodes = lm_graphs[i]->get_nodes();
        set<LandmarkNode *>::const_iterator it;
        for (it = nodes.begin(); it != nodes.end(); ++it) {
            const LandmarkNode &node = **it;
            const FactPair &lm_fact = node.facts[0];
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
                set<FactPair> lm_facts;
                bool exists = false;
                for (const FactPair &lm_fact: node.facts) {
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
                utils::exit_with(ExitCode::UNSUPPORTED);
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
                    EdgeType e_type = to.second;
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

bool LandmarkFactoryMerged::supports_conditional_effects() const {
    for (const LandmarkFactory *lm_factory : lm_factories) {
        if (!lm_factory->supports_conditional_effects()) {
            return false;
        }
    }
    return true;
}

static LandmarkFactory *_parse(OptionParser &parser) {
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
    parser.add_list_option<LandmarkFactory *>("lm_factories");
    _add_options_to_parser(parser);
    Options opts = parser.parse();

    opts.verify_list_non_empty<LandmarkFactory *>("lm_factories");

    parser.document_language_support("conditional_effects",
                                     "supported if all components support them");

    if (parser.dry_run()) {
        return nullptr;
    } else {
        return new LandmarkFactoryMerged(opts);
    }
}

static Plugin<LandmarkFactory> _plugin(
    "lm_merged", _parse);
}
