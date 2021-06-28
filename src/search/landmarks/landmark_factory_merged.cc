#include "landmark_factory_merged.h"

#include "landmark.h"
#include "landmark_graph.h"

#include "../option_parser.h"
#include "../plugin.h"

#include "../utils/logging.h"

#include <set>

using namespace std;
using utils::ExitCode;

namespace landmarks {
class LandmarkNode;

LandmarkFactoryMerged::LandmarkFactoryMerged(const Options &opts)
    : lm_factories(opts.get_list<shared_ptr<LandmarkFactory>>("lm_factories")) {
}

LandmarkNode *LandmarkFactoryMerged::get_matching_landmark(const Landmark &landmark) const {
    if (!landmark.disjunctive && !landmark.conjunctive) {
        const FactPair &lm_fact = landmark.facts[0];
        if (lm_graph->contains_simple_landmark(lm_fact))
            return &lm_graph->get_simple_landmark(lm_fact);
        else
            return 0;
    } else if (landmark.disjunctive) {
        set<FactPair> lm_facts(landmark.facts.begin(), landmark.facts.end());
        if (lm_graph->contains_identical_disjunctive_landmark(lm_facts))
            return &lm_graph->get_disjunctive_landmark(landmark.facts[0]);
        else
            return 0;
    } else if (landmark.conjunctive) {
        cerr << "Don't know how to handle conjunctive landmarks yet" << endl;
        utils::exit_with(ExitCode::SEARCH_UNSUPPORTED);
    }
    return 0;
}

void LandmarkFactoryMerged::generate_landmarks(
    const shared_ptr<AbstractTask> &task) {
    utils::g_log << "Merging " << lm_factories.size() << " landmark graphs" << endl;

    vector<shared_ptr<LandmarkGraph>> lm_graphs;
    lm_graphs.reserve(lm_factories.size());
    for (const shared_ptr<LandmarkFactory> &lm_factory : lm_factories) {
        lm_graphs.push_back(lm_factory->compute_lm_graph(task));
    }

    utils::g_log << "Adding simple landmarks" << endl;
    for (size_t i = 0; i < lm_graphs.size(); ++i) {
        const LandmarkGraph::Nodes &nodes = lm_graphs[i]->get_nodes();
        // TODO: loop over landmarks instead
        for (auto &lm_node : nodes) {
            const Landmark &landmark = lm_node->get_landmark();
            if (landmark.conjunctive) {
                cerr << "Don't know how to handle conjunctive landmarks yet" << endl;
                utils::exit_with(ExitCode::SEARCH_UNSUPPORTED);
            } else if (landmark.disjunctive) {
                continue;
            } else if (!lm_graph->contains_landmark(landmark.facts[0])) {
                Landmark copy(landmark);
                lm_graph->add_landmark(move(copy));
            }
        }
    }

    utils::g_log << "Adding disjunctive landmarks" << endl;
    for (size_t i = 0; i < lm_graphs.size(); ++i) {
        const LandmarkGraph::Nodes &nodes = lm_graphs[i]->get_nodes();
        for (auto &lm_node : nodes) {
            const Landmark &landmark = lm_node->get_landmark();
            if (landmark.disjunctive) {
/*
  TODO: It seems that disjunctive landmarks are only added if none of the
   facts it is made of is also there as a simple landmark. This should
   either be more general (add only if none of its subset is already there)
   or it should be done only upon request (e.g., heuristics that consider
   orders might want to keep all landmarks).
*/
                bool exists =
                    any_of(landmark.facts.begin(), landmark.facts.end(),
                           [&](const FactPair &lm_fact) {return lm_graph->contains_landmark(lm_fact);});
                if (!exists) {
                    Landmark copy(landmark);
                    lm_graph->add_landmark(move(copy));
                }
            }
        }
    }

    utils::g_log << "Adding orderings" << endl;
    for (size_t i = 0; i < lm_graphs.size(); ++i) {
        const LandmarkGraph::Nodes &nodes = lm_graphs[i]->get_nodes();
        for (auto &from_orig : nodes) {
            LandmarkNode *from = get_matching_landmark(from_orig->get_landmark());
            if (from) {
                for (const auto &to : from_orig->children) {
                    const LandmarkNode *to_orig = to.first;
                    EdgeType e_type = to.second;
                    LandmarkNode *to_node = get_matching_landmark(to_orig->get_landmark());
                    if (to_node) {
                        edge_add(*from, *to_node, e_type);
                    } else {
                        utils::g_log << "Discarded to ordering" << endl;
                    }
                }
            } else {
                utils::g_log << "Discarded from ordering" << endl;
            }
        }
    }
    postprocess();
}

void LandmarkFactoryMerged::postprocess() {
    lm_graph->set_landmark_ids();
    mk_acyclic_graph();
}

bool LandmarkFactoryMerged::computes_reasonable_orders() const {
    for (const shared_ptr<LandmarkFactory> &lm_factory : lm_factories) {
        if (lm_factory->computes_reasonable_orders()) {
            return true;
        }
    }
    return false;
}

bool LandmarkFactoryMerged::supports_conditional_effects() const {
    for (const shared_ptr<LandmarkFactory> &lm_factory : lm_factories) {
        if (!lm_factory->supports_conditional_effects()) {
            return false;
        }
    }
    return true;
}

static shared_ptr<LandmarkFactory> _parse(OptionParser &parser) {
    parser.document_synopsis(
        "Merged Landmarks",
        "Merges the landmarks and orderings from the parameter landmarks");
    parser.document_note(
        "Precedence",
        "Fact landmarks take precedence over disjunctive landmarks, "
        "orderings take precedence in the usual manner "
        "(gn > nat > reas > o_reas). ");
    parser.document_note(
        "Note",
        "Does not currently support conjunctive landmarks");
    parser.add_list_option<shared_ptr<LandmarkFactory>>("lm_factories");
    Options opts = parser.parse();

    opts.verify_list_non_empty<shared_ptr<LandmarkFactory>>("lm_factories");

    parser.document_language_support("conditional_effects",
                                     "supported if all components support them");

    if (parser.dry_run())
        return nullptr;
    else
        return make_shared<LandmarkFactoryMerged>(opts);
}

static Plugin<LandmarkFactory> _plugin("lm_merged", _parse);
}
