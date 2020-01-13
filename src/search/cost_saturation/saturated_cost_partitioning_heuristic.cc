#include "abstraction.h"
#include "cost_partitioning_heuristic.h"
#include "utils.h"

#include "../option_parser.h"
#include "../plugin.h"

#include "../utils/markup.h"

using namespace std;

namespace cost_saturation {
static CostPartitioningHeuristic compute_saturated_cost_partitioning(
    const Abstractions &abstractions,
    const vector<int> &order,
    const vector<int> &costs) {
    assert(abstractions.size() == order.size());
    CostPartitioningHeuristic cp_heuristic;
    vector<int> remaining_costs = costs;
    for (int pos : order) {
        const Abstraction &abstraction = *abstractions[pos];
        vector<int> h_values = abstraction.compute_goal_distances(remaining_costs);
        vector<int> saturated_costs = abstraction.compute_saturated_costs(h_values);
        cp_heuristic.add_h_values(pos, move(h_values));
        reduce_costs(remaining_costs, saturated_costs);
    }
    return cp_heuristic;
}

static shared_ptr<Evaluator> _parse(OptionParser &parser) {
    parser.document_synopsis(
        "Saturated cost partitioning",
        "Compute the maximum over multiple saturated cost partitioning "
        "heuristics using different orders. Depending on the options, orders "
        "may be greedy, optimized and/or diverse. We describe saturated cost "
        "partitioning in the paper" +
        utils::format_journal_reference(
            {"Jendrik Seipp", "Malte Helmert"},
            "Counterexample-Guided Cartesian Abstraction Refinement for "
            "Classical Planning",
            "https://ai.dmi.unibas.ch/papers/seipp-helmert-jair2018.pdf",
            "Journal of Artificial Intelligence Research",
            "62",
            "535-577",
            "2018") +
        "and show how to compute saturated cost partitioning heuristics for "
        "multiple (diverse) orders in" +
        utils::format_conference_reference(
            {"Jendrik Seipp", "Thomas Keller", "Malte Helmert"},
            "Narrowing the Gap Between Saturated and Optimal Cost Partitioning "
            "for Classical Planning",
            "https://ai.dmi.unibas.ch/papers/seipp-et-al-aaai2017.pdf",
            "Proceedings of the 31st AAAI Conference on Artificial Intelligence "
            "(AAAI 2017)",
            "3651-3657",
            "AAAI Press",
            "2017") +
        "Greedy orders for saturated cost partitioning are introduced in" +
        utils::format_conference_reference(
            {"Jendrik Seipp"},
            "Better Orders for Saturated Cost Partitioning in Optimal "
            "Classical Planning",
            "https://ai.dmi.unibas.ch/papers/seipp-socs2017.pdf",
            "Proceedings of the 10th Annual Symposium on Combinatorial Search "
            "(SoCS 2017)",
            "149-153",
            "AAAI Press",
            "2017"));
    parser.document_note(
        "Difference to cegar()",
        "The cegar() plugin computes a single saturated cost partitioning over "
        "Cartesian abstraction heuristics. In contrast, "
        "saturated_cost_partitioning() supports computing the maximum over "
        "multiple saturated cost partitionings using different heuristic "
        "orders, and it supports both Cartesian abstraction heuristics and "
        "pattern database heuristics. While cegar() interleaves abstraction "
        "computation with cost partitioning, saturated_cost_partitioning() "
        "computes all abstractions using the original costs.");
    return get_max_cp_heuristic(parser, compute_saturated_cost_partitioning);
}

static Plugin<Evaluator> _plugin("scp", _parse);
}
