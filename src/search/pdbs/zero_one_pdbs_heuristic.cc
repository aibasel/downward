#include "zero_one_pdbs_heuristic.h"

#include "../plugins/plugin.h"
#include "../utils/markup.h"

#include <limits>

using namespace std;

namespace pdbs {
static ZeroOnePDBs get_zero_one_pdbs_from_generator(
    const shared_ptr<AbstractTask> &task,
    const shared_ptr<PatternCollectionGenerator> &pattern_generator) {
    PatternCollectionInformation pattern_collection_info =
        pattern_generator->generate(task);
    shared_ptr<PatternCollection> patterns =
        pattern_collection_info.get_patterns();
    TaskProxy task_proxy(*task);
    return ZeroOnePDBs(task_proxy, *patterns);
}

ZeroOnePDBsHeuristic::ZeroOnePDBsHeuristic(
    const shared_ptr<AbstractTask> &task,
    const shared_ptr<PatternCollectionGenerator> &patterns,
    bool cache_estimates, const string &description, utils::Verbosity verbosity)
    : Heuristic(task, cache_estimates, description, verbosity),
      zero_one_pdbs(get_zero_one_pdbs_from_generator(task, patterns)) {
}

int ZeroOnePDBsHeuristic::compute_heuristic(const State &ancestor_state) {
    State state = convert_ancestor_state(ancestor_state);
    int h = zero_one_pdbs.get_value(state);
    if (h == numeric_limits<int>::max())
        return DEAD_END;
    return h;
}

class ZeroOnePDBsHeuristicFeature
    : public plugins::TypedFeature<TaskIndependentEvaluator> {
public:
    ZeroOnePDBsHeuristicFeature() : TypedFeature("zopdbs") {
        document_subcategory("heuristics_pdb");
        document_title("Zero-One PDB");
        document_note(
            "Reference",
            "The idea of zero-one cost partitioning is described in the "
            "following paper:" +
                utils::format_conference_reference(
                    {"Patrik Haslum", "Blai Bonet", "Hector Geffner"},
                    "New Admissible Heuristics for Domain-Independent "
                    "Planning",
                    "https://cdn.aaai.org/AAAI/2005/AAAI05-184.pdf",
                    "Proceedings of the 20th National Conference on "
                    "Artificial Intelligence (AAAI 2005)",
                    "1163-1168", "AAAI Press", "2005") +
                "Greedy zero-one cost partitioning for pattern databases is "
                "described in the following paper:" +
                utils::format_conference_reference(
                    {"Stefan Edelkamp"},
                    "Symbolic Pattern Databases in Heuristic Search Planning",
                    "https://cdn.aaai.org/AIPS/2002/AIPS02-028.pdf",
                    "Proceedings of the Sixth International Conference on "
                    "Artificial Intelligence Planning Systems "
                    "(AIPS 2002)",
                    "274-283", "AAAI Press", "2002") +
                "It is discussed in the context of other cost partitioning "
                "methods in the following paper:" +
                utils::format_conference_reference(
                    {"Jendrik Seipp", "Thomas Keller", "Malte Helmert"},
                    "A Comparison of Cost Partitioning Algorithms for Optimal "
                    "Classical Planning",
                    "https://ai.dmi.unibas.ch/papers/seipp-et-al-icaps2017.pdf",
                    "Proceedings of the 27th International Conference on "
                    "Automated Planning and Scheduling (ICAPS 2017)",
                    "259-268", "AAAI Press", "2017") +
                "For implementation notes, see:" +
                utils::format_conference_reference(
                    {"Silvan Sievers", "Manuela Ortlieb", "Malte Helmert"},
                    "Efficient Implementation of Pattern Database Heuristics "
                    "for Classical Planning",
                    "https://ai.dmi.unibas.ch/papers/sievers-et-al-socs2012.pdf",
                    "Proceedings of the Fifth Annual Symposium on "
                    "Combinatorial Search (SoCS 2012)",
                    "49-56", "AAAI Press", "2012"));
        document_synopsis(
            "The zero/one pattern database heuristic is simply the sum of the "
            "heuristic values of all patterns in the pattern collection. In contrast "
            "to the canonical pattern database heuristic, there is no need to check "
            "for additive subsets, because the additivity of the patterns is "
            "guaranteed by action cost partitioning. This heuristic uses the most "
            "simple form of action cost partitioning, i.e. if an operator affects "
            "more than one pattern in the collection, its costs are entirely taken "
            "into account for one pattern (the first one which it affects) and set "
            "to zero for all other affected patterns.");

        add_option<shared_ptr<TaskIndependentPatternCollectionGenerator>>(
            "patterns", "pattern generation method", "systematic(1)");
        add_heuristic_options_to_feature(*this, "zopdbs");

        document_language_support("action costs", "supported");
        document_language_support("conditional effects", "not supported");
        document_language_support("axioms", "not supported");

        document_property("admissible", "yes");
        document_property("consistent", "yes");
        document_property("safe", "yes");
        document_property("preferred operators", "no");
    }

    virtual shared_ptr<TaskIndependentEvaluator> create_component(
        const plugins::Options &opts) const override {
        return components::make_auto_task_independent_component<
            ZeroOnePDBsHeuristic, Evaluator>(
            opts.get<shared_ptr<TaskIndependentPatternCollectionGenerator>>(
                "patterns"),
            get_heuristic_arguments_from_options(opts));
    }
};

static plugins::FeaturePlugin<ZeroOnePDBsHeuristicFeature> _plugin;
}
