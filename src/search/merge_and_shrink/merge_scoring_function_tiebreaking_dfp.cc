#include "merge_scoring_function_tiebreaking_dfp.h"

#include "factored_transition_system.h"
#include "transition_system.h"

#include "../task_proxy.h"

#include "../options/option_parser.h"
#include "../options/options.h"
#include "../options/plugin.h"

#include "../utils/markup.h"
#include "../utils/rng.h"
#include "../utils/rng_options.h"

#include <cassert>

using namespace std;

namespace merge_and_shrink {
MergeScoringFunctionTiebreakingDFP::MergeScoringFunctionTiebreakingDFP(
    const options::Options &options)
    : atomic_ts_order(AtomicTSOrder(options.get_enum("atomic_ts_order"))),
      product_ts_order(ProductTSOrder(options.get_enum("product_ts_order"))),
      atomic_before_product(options.get<bool>("atomic_before_product")),
      random_seed(options.get<int>("random_seed")) {
      rng = utils::parse_rng_from_options(options);
}

vector<int> MergeScoringFunctionTiebreakingDFP::compute_scores(
    FactoredTransitionSystem &,
    const vector<pair<int, int>> &merge_candidates) {
    vector<int> scores;
    for (size_t candidate_index = 0; candidate_index < merge_candidates.size();
         ++candidate_index) {
        pair<int, int> merge_candidate = merge_candidates[candidate_index];
        int ts_index1 = merge_candidate.first;
        int ts_index2 = merge_candidate.second;
        for (size_t merge_candidate_order_index = 0;
             merge_candidate_order_index < merge_candidate_order.size();
             ++merge_candidate_order_index) {
            pair<int, int> other_candidate =
                merge_candidate_order[merge_candidate_order_index];
            if ((other_candidate.first == ts_index1 &&
                    other_candidate.second == ts_index2) ||
                (other_candidate.second == ts_index1 &&
                    other_candidate.first == ts_index2)) {
                // use the index in the merge candidate order as score
                scores.push_back(merge_candidate_order_index);
                // TODO: remove debug output
//                cout << "pair " << ts_index1 << ", " << ts_index2 <<
//                        " ordered number " << merge_candidate_order_index <<
//                        " in merge candidates order" << endl;
            }
        }
    }
    return scores;
}

void MergeScoringFunctionTiebreakingDFP::initialize(
    shared_ptr<AbstractTask> task) {
    TaskProxy task_proxy(*task);
    int num_variables = task_proxy.get_variables().size();
    int max_transition_system_count = num_variables * 2 - 1;
    vector<int> transition_system_order;
    transition_system_order.reserve(max_transition_system_count);

    // Compute the order in which atomic transition systems are considered
    vector<int> atomic_tso;
    for (int i = 0; i < num_variables; ++i) {
        atomic_tso.push_back(i);
    }
    if (atomic_ts_order == AtomicTSOrder::INVERSE) {
        reverse(atomic_tso.begin(), atomic_tso.end());
    } else if (atomic_ts_order == AtomicTSOrder::RANDOM) {
        rng->shuffle(atomic_tso);
    }

    // Compute the order in which product transition systems are considered
    vector<int> product_tso;
    for (int i = num_variables; i < max_transition_system_count; ++i) {
        product_tso.push_back(i);
    }
    if (product_ts_order == ProductTSOrder::NEW_TO_OLD) {
        reverse(product_tso.begin(), product_tso.end());
    } else if (product_ts_order == ProductTSOrder::RANDOM) {
        rng->shuffle(product_tso);
    }

    // Put the orders in the correct order
    if (atomic_before_product) {
        transition_system_order.insert(transition_system_order.end(),
                                       atomic_tso.begin(),
                                       atomic_tso.end());
        transition_system_order.insert(transition_system_order.end(),
                                       product_tso.begin(),
                                       product_tso.end());
    } else {
        transition_system_order.insert(transition_system_order.end(),
                                       product_tso.begin(),
                                       product_tso.end());
        transition_system_order.insert(transition_system_order.end(),
                                       atomic_tso.begin(),
                                       atomic_tso.end());
    }

    merge_candidate_order.reserve(max_transition_system_count *
                                  max_transition_system_count / 2);
    for (size_t i = 0; i < transition_system_order.size(); ++i) {
        for (size_t j = i + 1; j < transition_system_order.size(); ++j) {
            merge_candidate_order.emplace_back(
                transition_system_order[i], transition_system_order[j]);
        }
    }
}

void MergeScoringFunctionTiebreakingDFP::dump_specific_options() const {
    cout << "Atomic transition system order: ";
    switch (atomic_ts_order) {
    case AtomicTSOrder::REGULAR:
        cout << "regular";
        break;
    case AtomicTSOrder::INVERSE:
        cout << "inverse";
        break;
    case AtomicTSOrder::RANDOM:
        cout << "random";
        break;
    }
    cout << endl;

    cout << "Product transition system order: ";
    switch (product_ts_order) {
    case ProductTSOrder::OLD_TO_NEW:
        cout << "old to new";
        break;
    case ProductTSOrder::NEW_TO_OLD:
        cout << "new to old";
        break;
    case ProductTSOrder::RANDOM:
        cout << "random";
        break;
    }
    cout << endl;

    cout << "Consider " << (atomic_before_product ?
                            "atomic before product" : "product before atomic")
         << " transition systems" << endl;
    cout << "Random seed: " << random_seed << endl;
}

void MergeScoringFunctionTiebreakingDFP::add_options_to_parser(
    options::OptionParser &parser) {
    vector<string> atomic_ts_order;
    vector<string> atomic_ts_order_documentation;
    atomic_ts_order.push_back("regular");
    atomic_ts_order_documentation.push_back(
        "the variable order of Fast Downward");
    atomic_ts_order.push_back("inverse");
    atomic_ts_order_documentation.push_back("opposite of regular");
    atomic_ts_order.push_back("random");
    atomic_ts_order_documentation.push_back("a randomized order");
    parser.add_enum_option(
        "atomic_ts_order",
        atomic_ts_order,
        "The order in which atomic transition systems are considered when "
        "considering pairs of potential merges.",
        "regular",
        atomic_ts_order_documentation);

    vector<string> product_ts_order;
    vector<string> product_ts_order_documentation;
    product_ts_order.push_back("old_to_new");
    product_ts_order_documentation.push_back(
        "consider composite transition systems from most recent to oldest, "
        "that is in decreasing index order");
    product_ts_order.push_back("new_to_old");
    product_ts_order_documentation.push_back("opposite of old_to_new");
    product_ts_order.push_back("random");
    product_ts_order_documentation.push_back("a randomized order");
    parser.add_enum_option(
        "product_ts_order",
        product_ts_order,
        "The order in which product transition systems are considered when "
        "considering pairs of potential merges.",
        "new_to_old",
        product_ts_order_documentation);

    parser.add_option<bool>(
        "atomic_before_product",
        "Consider atomic transition systems before composite ones iff true.",
        "false");

    utils::add_rng_options(parser);
}

static shared_ptr<MergeScoringFunction>_parse(options::OptionParser &parser) {
    parser.document_synopsis(
        "DFP tiebreaking",
        "This scoring function computes an order on merge candidates, based"
        "on an order of all possible 'indices' of transition systems that may "
        "occur. The score for each merge candidate correponds to its position "
        "in the order. This tiebreaking has been introduced in the following "
        "paper:"
            + utils::format_paper_reference(
            {"Silvan Sievers", "Martin Wehrle", "Malte Helmert"},
            "An Analysis of Merge Strategies for Merge-and-Shrink Heuristics",
            "http://ai.cs.unibas.ch/papers/sievers-et-al-icaps2016.pdf",
            "Proceedings of the 26th International Conference on Automated "
            "Planning and Scheduling (ICAPS 2016)",
            "294-298",
            "AAAI Press 2016"));
    MergeScoringFunctionTiebreakingDFP::add_options_to_parser(parser);

    options::Options options = parser.parse();
    if (parser.dry_run())
        return nullptr;
    else
        return make_shared<MergeScoringFunctionTiebreakingDFP>(options);
}

static options::PluginShared<MergeScoringFunction> _plugin("tiebreaking_dfp", _parse);
}
