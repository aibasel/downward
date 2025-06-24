#include "landmark_factory_merged.h"

#include "landmark.h"
#include "landmark_graph.h"

#include "../utils/component_errors.h"
#include "../plugins/plugin.h"

#include "util.h"

#include <ranges>

using namespace std;
using utils::ExitCode;

namespace landmarks {
class LandmarkNode;

LandmarkFactoryMerged::LandmarkFactoryMerged(
    const vector<shared_ptr<LandmarkFactory>> &lm_factories,
    utils::Verbosity verbosity)
    : LandmarkFactory(verbosity),
      landmark_factories(lm_factories) {
    utils::verify_list_not_empty(lm_factories, "lm_factories");
}

LandmarkNode *LandmarkFactoryMerged::get_matching_landmark(
    const Landmark &landmark) const {
    if (landmark.type == DISJUNCTIVE) {
        utils::HashSet<FactPair> atoms(
            landmark.atoms.begin(), landmark.atoms.end());
        if (landmark_graph->contains_superset_disjunctive_landmark(atoms)) {
            return &landmark_graph->get_disjunctive_landmark_node(
                landmark.atoms[0]);
        }
        return nullptr;
    }

    if (landmark.type == CONJUNCTIVE) {
        cerr << "Don't know how to handle conjunctive landmarks yet..." << endl;
        utils::exit_with(ExitCode::SEARCH_UNSUPPORTED);
    }

    assert(landmark.atoms.size() == 1);
    const FactPair &atom = landmark.atoms[0];
    if (landmark_graph->contains_atomic_landmark(atom)) {
        return &landmark_graph->get_atomic_landmark_node(atom);
    }
    return nullptr;
}

vector<shared_ptr<LandmarkGraph>> LandmarkFactoryMerged::generate_landmark_graphs_of_subfactories(
    const shared_ptr<AbstractTask> &task) {
    vector<shared_ptr<LandmarkGraph>> landmark_graphs;
    landmark_graphs.reserve(landmark_factories.size());
    achievers_calculated = true;
    for (const shared_ptr<LandmarkFactory> &landmark_factory : landmark_factories) {
        landmark_graphs.push_back(
            landmark_factory->compute_landmark_graph(task));
        achievers_calculated &= landmark_factory->achievers_are_calculated();
    }
    return landmark_graphs;
}

void LandmarkFactoryMerged::add_atomic_landmarks(
    const vector<shared_ptr<LandmarkGraph>> &landmark_graphs) const {
    if (log.is_at_least_normal()) {
        log << "Adding atomic landmarks" << endl;
    }
    for (const auto &graph_to_merge : landmark_graphs) {
        // TODO: Loop over landmarks instead.
        for (const auto &node : *graph_to_merge) {
            const Landmark &landmark = node->get_landmark();
            if (landmark.type == CONJUNCTIVE) {
                cerr << "Don't know how to handle conjunctive landmarks yet"
                     << endl;
                utils::exit_with(ExitCode::SEARCH_UNSUPPORTED);
            }
            if (landmark.type == DISJUNCTIVE) {
                continue;
            }
            assert(landmark.atoms.size() == 1);
            if (!landmark_graph->contains_landmark(landmark.atoms[0])) {
                Landmark copy(landmark);
                landmark_graph->add_landmark(move(copy));
            }
        }
    }
}

void LandmarkFactoryMerged::add_disjunctive_landmarks(
    const vector<shared_ptr<LandmarkGraph>> &landmark_graphs) const {
    if (log.is_at_least_normal()) {
        log << "Adding disjunctive landmarks" << endl;
    }
    for (const shared_ptr<LandmarkGraph> &graph_to_merge : landmark_graphs) {
        for (const auto &node : *graph_to_merge) {
            const Landmark &landmark = node->get_landmark();
            if (landmark.type == DISJUNCTIVE) {
                /*
                  TODO: It seems that disjunctive landmarks are only added if
                  none of the atoms it is made of is also there as an atomic
                  landmark. This should either be more general (add only if none
                  of its subset is already there) or it should be done only upon
                  request (e.g., heuristics that consider orders might want to
                  keep all landmarks).
                */
                bool exists = ranges::any_of(
                    landmark.atoms, [&](const FactPair &atom) {
                        return landmark_graph->contains_landmark(atom);
                    });
                if (!exists) {
                    Landmark copy(landmark);
                    landmark_graph->add_landmark(move(copy));
                }
            }
        }
    }
}

void LandmarkFactoryMerged::add_landmark_orderings(
    const vector<shared_ptr<LandmarkGraph>> &landmark_graphs) const {
    if (log.is_at_least_normal()) {
        log << "Adding orderings" << endl;
    }
    for (const auto &landmark_graph : landmark_graphs) {
        for (const auto &from_old : *landmark_graph) {
            LandmarkNode *from_new =
                get_matching_landmark(from_old->get_landmark());
            if (from_new) {
                for (const auto &[to_old, type] : from_old->children) {
                    LandmarkNode *to_new =
                        get_matching_landmark(to_old->get_landmark());
                    if (to_new) {
                        add_or_replace_ordering_if_stronger(
                            *from_new, *to_new, type);
                    } else if (log.is_at_least_normal()) {
                        log << "Discarded to ordering" << endl;
                    }
                }
            } else if (log.is_at_least_normal()) {
                log << "Discarded from ordering" << endl;
            }
        }
    }
}

void LandmarkFactoryMerged::generate_landmarks(
    const shared_ptr<AbstractTask> &task) {
    if (log.is_at_least_normal()) {
        log << "Merging " << landmark_factories.size()
            << " landmark graphs" << endl;
    }
    vector<shared_ptr<LandmarkGraph>> landmark_graphs =
        generate_landmark_graphs_of_subfactories(task);
    add_atomic_landmarks(landmark_graphs);
    add_disjunctive_landmarks(landmark_graphs);
    add_landmark_orderings(landmark_graphs);
    postprocess();
}

void LandmarkFactoryMerged::postprocess() {
    landmark_graph->set_landmark_ids();
}

bool LandmarkFactoryMerged::supports_conditional_effects() const {
    return ranges::all_of(
        landmark_factories,
        [&](const shared_ptr<LandmarkFactory> &landmark_factory) {
            return landmark_factory->supports_conditional_effects();
        });
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

    virtual shared_ptr<LandmarkFactoryMerged> create_component(
        const plugins::Options &opts) const override {
        return plugins::make_shared_from_arg_tuples<LandmarkFactoryMerged>(
            opts.get_list<shared_ptr<LandmarkFactory>>("lm_factories"),
            get_landmark_factory_arguments_from_options(opts));
    }
};

static plugins::FeaturePlugin<LandmarkFactoryMergedFeature> _plugin;
}
