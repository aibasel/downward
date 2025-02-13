#include "landmark_factory_merged.h"

#include "landmark.h"
#include "landmark_graph.h"

#include "../utils/component_errors.h"
#include "../plugins/plugin.h"

#include <set>

using namespace std;
using utils::ExitCode;

namespace landmarks {
class LandmarkNode;

LandmarkFactoryMerged::LandmarkFactoryMerged(
    const vector<shared_ptr<LandmarkFactory>> &lm_factories,
    utils::Verbosity verbosity)
    : LandmarkFactory(verbosity),
      lm_factories(lm_factories) {
    utils::verify_list_not_empty(lm_factories, "lm_factories");
}

LandmarkNode *LandmarkFactoryMerged::get_matching_landmark(const Landmark &landmark) const {
    if (!landmark.disjunctive && !landmark.conjunctive) {
        const FactPair &lm_fact = landmark.facts[0];
        if (lm_graph->contains_simple_landmark(lm_fact))
            return &lm_graph->get_simple_landmark(lm_fact);
        else
            return nullptr;
    } else if (landmark.disjunctive) {
        set<FactPair> lm_facts(landmark.facts.begin(), landmark.facts.end());
        if (lm_graph->contains_identical_disjunctive_landmark(lm_facts))
            return &lm_graph->get_disjunctive_landmark(landmark.facts[0]);
        else
            return nullptr;
    } else if (landmark.conjunctive) {
        cerr << "Don't know how to handle conjunctive landmarks yet" << endl;
        utils::exit_with(ExitCode::SEARCH_UNSUPPORTED);
    }
    return nullptr;
}

void LandmarkFactoryMerged::generate_landmarks(
    const shared_ptr<AbstractTask> &task) {
    if (log.is_at_least_normal()) {
        log << "Merging " << lm_factories.size() << " landmark graphs" << endl;
    }

    vector<shared_ptr<LandmarkGraph>> lm_graphs;
    lm_graphs.reserve(lm_factories.size());
    achievers_calculated = true;
    for (const shared_ptr<LandmarkFactory> &lm_factory : lm_factories) {
        lm_graphs.push_back(lm_factory->compute_lm_graph(task));
        achievers_calculated &= lm_factory->achievers_are_calculated();
    }

    if (log.is_at_least_normal()) {
        log << "Adding simple landmarks" << endl;
    }
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

    if (log.is_at_least_normal()) {
        log << "Adding disjunctive landmarks" << endl;
    }
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

    if (log.is_at_least_normal()) {
        log << "Adding orderings" << endl;
    }
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
                        if (log.is_at_least_normal()) {
                            log << "Discarded to ordering" << endl;
                        }
                    }
                }
            } else {
                if (log.is_at_least_normal()) {
                    log << "Discarded from ordering" << endl;
                }
            }
        }
    }
    postprocess();
}

void LandmarkFactoryMerged::postprocess() {
    lm_graph->set_landmark_ids();
}

bool LandmarkFactoryMerged::supports_conditional_effects() const {
    for (const shared_ptr<LandmarkFactory> &lm_factory : lm_factories) {
        if (!lm_factory->supports_conditional_effects()) {
            return false;
        }
    }
    return true;
}

class LandmarkFactoryMergedFeature
    : public plugins::TypedFeature<LandmarkFactory, LandmarkFactoryMerged> {
public:
    LandmarkFactoryMergedFeature() : TypedFeature("lm_merged") {
        document_title("Merged Landmarks");
        document_synopsis(
            "Merges the landmarks and orderings from the parameter landmarks");

        add_list_option<shared_ptr<LandmarkFactory>>("lm_factories");
        add_landmark_factory_options_to_feature(*this);

        document_note(
            "Precedence",
            "Fact landmarks take precedence over disjunctive landmarks, "
            "orderings take precedence in the usual manner "
            "(gn > nat > reas > o_reas). ");
        document_note(
            "Note",
            "Does not currently support conjunctive landmarks");

        document_language_support(
            "conditional_effects",
            "supported if all components support them");
    }

    virtual shared_ptr<LandmarkFactoryMerged>
    create_component(const plugins::Options &opts) const override {
        return plugins::make_shared_from_arg_tuples<LandmarkFactoryMerged>(
            opts.get_list<shared_ptr<LandmarkFactory>>("lm_factories"),
            get_landmark_factory_arguments_from_options(opts));
    }
};

static plugins::FeaturePlugin<LandmarkFactoryMergedFeature> _plugin;
}
