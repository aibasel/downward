#include "additive_cartesian_heuristic.h"

#include "cartesian_heuristic_function.h"
#include "cost_saturation.h"
#include "types.h"
#include "utils.h"

#include "../plugins/plugin.h"
#include "../utils/logging.h"
#include "../utils/markup.h"
#include "../utils/rng.h"
#include "../utils/rng_options.h"

#include <cassert>

using namespace std;

namespace cartesian_abstractions {
static vector<CartesianHeuristicFunction> generate_heuristic_functions(
    const vector<shared_ptr<SubtaskGenerator>> &subtask_generators,
    int max_states, int max_transitions, double max_time,
    PickSplit pick, bool use_general_costs, int random_seed,
    const shared_ptr<AbstractTask> &transform, utils::LogProxy &log) {
    if (log.is_at_least_normal()) {
        log << "Initializing additive Cartesian heuristic..." << endl;
    }
    shared_ptr<utils::RandomNumberGenerator> rng =
        utils::get_rng(random_seed);
    CostSaturation cost_saturation(
        subtask_generators, max_states, max_transitions, max_time, pick,
        use_general_costs, *rng, log);
    return cost_saturation.generate_heuristic_functions(transform);
}

AdditiveCartesianHeuristic::AdditiveCartesianHeuristic(
    const vector<shared_ptr<SubtaskGenerator>> &subtasks,
    int max_states, int max_transitions, double max_time,
    PickSplit pick, bool use_general_costs, int random_seed,
    const shared_ptr<AbstractTask> &transform, bool cache_estimates,
    const string &description, utils::Verbosity verbosity)
    : Heuristic(transform, cache_estimates, description, verbosity),
      heuristic_functions(generate_heuristic_functions(
                              subtasks, max_states, max_transitions,
                              max_time, pick, use_general_costs,
                              random_seed, transform, log)) {
}

int AdditiveCartesianHeuristic::compute_heuristic(const State &ancestor_state) {
    State state = convert_ancestor_state(ancestor_state);
    int sum_h = 0;
    for (const CartesianHeuristicFunction &function : heuristic_functions) {
        int value = function.get_value(state);
        assert(value >= 0);
        if (value == INF)
            return DEAD_END;
        sum_h += value;
    }
    assert(sum_h >= 0);
    return sum_h;
}

class AdditiveCartesianHeuristicFeature
    : public plugins::TypedFeature<Evaluator, AdditiveCartesianHeuristic> {
public:
    AdditiveCartesianHeuristicFeature() : TypedFeature("cegar") {
        document_title("Additive Cartesian CEGAR heuristic");
        document_synopsis(
            "See the paper introducing counterexample-guided Cartesian "
            "abstraction refinement (CEGAR) for classical planning:" +
            utils::format_conference_reference(
                {"Jendrik Seipp", "Malte Helmert"},
                "Counterexample-guided Cartesian Abstraction Refinement",
                "https://ai.dmi.unibas.ch/papers/seipp-helmert-icaps2013.pdf",
                "Proceedings of the 23rd International Conference on Automated "
                "Planning and Scheduling (ICAPS 2013)",
                "347-351",
                "AAAI Press",
                "2013") +
            "and the paper showing how to make the abstractions additive:" +
            utils::format_conference_reference(
                {"Jendrik Seipp", "Malte Helmert"},
                "Diverse and Additive Cartesian Abstraction Heuristics",
                "https://ai.dmi.unibas.ch/papers/seipp-helmert-icaps2014.pdf",
                "Proceedings of the 24th International Conference on "
                "Automated Planning and Scheduling (ICAPS 2014)",
                "289-297",
                "AAAI Press",
                "2014") +
            "For more details on Cartesian CEGAR and saturated cost partitioning, "
            "see the journal paper" +
            utils::format_journal_reference(
                {"Jendrik Seipp", "Malte Helmert"},
                "Counterexample-Guided Cartesian Abstraction Refinement for "
                "Classical Planning",
                "https://ai.dmi.unibas.ch/papers/seipp-helmert-jair2018.pdf",
                "Journal of Artificial Intelligence Research",
                "62",
                "535-577",
                "2018"));

        add_list_option<shared_ptr<SubtaskGenerator>>(
            "subtasks",
            "subtask generators",
            "[landmarks(),goals()]");
        add_option<int>(
            "max_states",
            "maximum sum of abstract states over all abstractions",
            "infinity",
            plugins::Bounds("1", "infinity"));
        add_option<int>(
            "max_transitions",
            "maximum sum of real transitions (excluding self-loops) over "
            " all abstractions",
            "1M",
            plugins::Bounds("0", "infinity"));
        add_option<double>(
            "max_time",
            "maximum time in seconds for building abstractions",
            "infinity",
            plugins::Bounds("0.0", "infinity"));
        add_option<PickSplit>(
            "pick",
            "how to choose on which variable to split the flaw state",
            "max_refined");
        add_option<bool>(
            "use_general_costs",
            "allow negative costs in cost partitioning",
            "true");
        utils::add_rng_options_to_feature(*this);
        add_heuristic_options_to_feature(*this, "cegar");

        document_language_support("action costs", "supported");
        document_language_support("conditional effects", "not supported");
        document_language_support("axioms", "not supported");

        document_property("admissible", "yes");
        document_property("consistent", "yes");
        document_property("safe", "yes");
        document_property("preferred operators", "no");
    }

    virtual shared_ptr<AdditiveCartesianHeuristic>
    create_component(const plugins::Options &opts) const override {
        return plugins::make_shared_from_arg_tuples<AdditiveCartesianHeuristic>(
            opts.get_list<shared_ptr<SubtaskGenerator>>("subtasks"),
            opts.get<int>("max_states"),
            opts.get<int>("max_transitions"),
            opts.get<double>("max_time"),
            opts.get<PickSplit>("pick"),
            opts.get<bool>("use_general_costs"),
            utils::get_rng_arguments_from_options(opts),
            get_heuristic_arguments_from_options(opts));
    }
};

static plugins::FeaturePlugin<AdditiveCartesianHeuristicFeature> _plugin;

static plugins::TypedEnumPlugin<PickSplit> _enum_plugin({
        {"random",
         "select a random variable (among all eligible variables)"},
        {"min_unwanted",
         "select an eligible variable which has the least unwanted values "
         "(number of values of v that land in the abstract state whose "
         "h-value will probably be raised) in the flaw state"},
        {"max_unwanted",
         "select an eligible variable which has the most unwanted values "
         "(number of values of v that land in the abstract state whose "
         "h-value will probably be raised) in the flaw state"},
        {"min_refined",
         "select an eligible variable which is the least refined "
         "(-1 * (remaining_values(v) / original_domain_size(v))) "
         "in the flaw state"},
        {"max_refined",
         "select an eligible variable which is the most refined "
         "(-1 * (remaining_values(v) / original_domain_size(v))) "
         "in the flaw state"},
        {"min_hadd",
         "select an eligible variable with minimal h^add(s_0) value "
         "over all facts that need to be removed from the flaw state"},
        {"max_hadd",
         "select an eligible variable with maximal h^add(s_0) value "
         "over all facts that need to be removed from the flaw state"}
    });
}
