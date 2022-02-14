#include "merge_scoring_function_total_order.h"

#include "factored_transition_system.h"
#include "transition_system.h"

#include "../task_proxy.h"

#include "../options/option_parser.h"
#include "../options/options.h"
#include "../options/plugin.h"

#include "../utils/logging.h"
#include "../utils/markup.h"
#include "../utils/rng.h"
#include "../utils/rng_options.h"

#include <cassert>

using namespace std;

namespace merge_and_shrink {
MergeScoringFunctionTotalOrder::MergeScoringFunctionTotalOrder(
    const options::Options &options)
    : atomic_ts_order(options.get<AtomicTSOrder>("atomic_ts_order")),
      product_ts_order(options.get<ProductTSOrder>("product_ts_order")),
      atomic_before_product(options.get<bool>("atomic_before_product")),
      random_seed(options.get<int>("random_seed")),
      rng(utils::parse_rng_from_options(options)) {
}

vector<double> MergeScoringFunctionTotalOrder::compute_scores(
    const FactoredTransitionSystem &,
    const vector<pair<int, int>> &merge_candidates) {
    assert(initialized);
    vector<double> scores;
    scores.reserve(merge_candidates.size());
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
                break;
            }
        }
        // We must have inserted a score for the current candidate.
        assert(scores.size() == candidate_index + 1);
    }
    return scores;
}

void MergeScoringFunctionTotalOrder::initialize(const TaskProxy &task_proxy) {
    initialized = true;
    int num_variables = task_proxy.get_variables().size();
    int max_transition_system_count = num_variables * 2 - 1;
    vector<int> transition_system_order;
    transition_system_order.reserve(max_transition_system_count);

    // Compute the order in which atomic transition systems are considered
    vector<int> atomic_tso;
    atomic_tso.reserve(num_variables);
    for (int i = 0; i < num_variables; ++i) {
        atomic_tso.push_back(i);
    }
    if (atomic_ts_order == AtomicTSOrder::LEVEL) {
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

string MergeScoringFunctionTotalOrder::name() const {
    return "total order";
}

void MergeScoringFunctionTotalOrder::dump_function_specific_options(utils::LogProxy &log) const {
    log << "Atomic transition system order: ";
    switch (atomic_ts_order) {
    case AtomicTSOrder::REVERSE_LEVEL:
        log << "reverse level";
        break;
    case AtomicTSOrder::LEVEL:
        log << "level";
        break;
    case AtomicTSOrder::RANDOM:
        log << "random";
        break;
    }
    log << endl;

    log << "Product transition system order: ";
    switch (product_ts_order) {
    case ProductTSOrder::OLD_TO_NEW:
        log << "old to new";
        break;
    case ProductTSOrder::NEW_TO_OLD:
        log << "new to old";
        break;
    case ProductTSOrder::RANDOM:
        log << "random";
        break;
    }
    log << endl;

    log << "Consider " << (atomic_before_product ?
                           "atomic before product" : "product before atomic")
        << " transition systems" << endl;
    log << "Random seed: " << random_seed << endl;
}

void MergeScoringFunctionTotalOrder::add_options_to_parser(
    options::OptionParser &parser) {
    vector<string> atomic_ts_order;
    vector<string> atomic_ts_order_documentation;
    atomic_ts_order.push_back("reverse_level");
    atomic_ts_order_documentation.push_back(
        "the variable order of Fast Downward");
    atomic_ts_order.push_back("level");
    atomic_ts_order_documentation.push_back("opposite of reverse_level");
    atomic_ts_order.push_back("random");
    atomic_ts_order_documentation.push_back("a randomized order");
    parser.add_enum_option<AtomicTSOrder>(
        "atomic_ts_order",
        atomic_ts_order,
        "The order in which atomic transition systems are considered when "
        "considering pairs of potential merges.",
        "reverse_level",
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
    parser.add_enum_option<ProductTSOrder>(
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
        "Total order",
        "This scoring function computes a total order on the merge candidates, "
        "based on the specified options. The score for each merge candidate "
        "correponds to its position in the order. This scoring function is "
        "mainly intended as tie-breaking, and has been introduced in the "
        "following paper:"
        + utils::format_conference_reference(
            {"Silvan Sievers", "Martin Wehrle", "Malte Helmert"},
            "An Analysis of Merge Strategies for Merge-and-Shrink Heuristics",
            "https://ai.dmi.unibas.ch/papers/sievers-et-al-icaps2016.pdf",
            "Proceedings of the 26th International Conference on Automated "
            "Planning and Scheduling (ICAPS 2016)",
            "294-298",
            "AAAI Press",
            "2016") +
        "Furthermore, using the atomic_ts_order option, this scoring function, "
        "if used alone in a score based filtering merge selector, can be used "
        "to emulate the corresponding (precomputed) linear merge strategies "
        "reverse level/level (independently of the other options).");
    MergeScoringFunctionTotalOrder::add_options_to_parser(parser);

    options::Options options = parser.parse();
    if (parser.dry_run())
        return nullptr;
    else
        return make_shared<MergeScoringFunctionTotalOrder>(options);
}

static options::Plugin<MergeScoringFunction> _plugin("total_order", _parse);
}
