#include "merge_strategy_factory_dfp.h"

#include "merge_dfp.h"

#include "../abstract_task.h"
#include "../task_proxy.h"

#include "../options/option_parser.h"
#include "../options/plugin.h"

#include "../utils/markup.h"
#include "../utils/memory.h"
#include "../utils/rng.h"
#include "../utils/rng_options.h"

#include <iostream>

using namespace std;

namespace merge_and_shrink {
MergeStrategyFactoryDFP::MergeStrategyFactoryDFP(const options::Options &options)
    : MergeStrategyFactory(),
      atomic_ts_order(AtomicTSOrder(options.get_enum("atomic_ts_order"))),
      product_ts_order(ProductTSOrder(options.get_enum("product_ts_order"))),
      atomic_before_product(options.get<bool>("atomic_before_product")),
      randomized_order(options.get<bool>("randomized_order")) {
    rng = utils::parse_rng_from_options(options);
}

unique_ptr<MergeStrategy> MergeStrategyFactoryDFP::compute_merge_strategy(
    std::shared_ptr<AbstractTask> task,
    FactoredTransitionSystem &fts) {
    vector<int> transition_system_order = compute_ts_order(task);
    return utils::make_unique_ptr<MergeDFP>(fts, move(transition_system_order));
}

vector<int> MergeStrategyFactoryDFP::compute_ts_order(
    shared_ptr<AbstractTask> task) {
    TaskProxy task_proxy(*task);
    int num_variables = task_proxy.get_variables().size();
    int max_transition_system_count = num_variables * 2 - 1;
    vector<int> transition_system_order;
    transition_system_order.reserve(max_transition_system_count);
    if (randomized_order) {
        for (int i = 0; i < max_transition_system_count; ++i) {
            transition_system_order.push_back(i);
        }
        rng->shuffle(transition_system_order);
    } else {
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
    }
    return transition_system_order;
}

void MergeStrategyFactoryDFP::dump_strategy_specific_options() const {
    if (randomized_order) {
        cout << "Completely randommized transition system order" << endl;
    } else {
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
    }
}

string MergeStrategyFactoryDFP::name() const {
    return "dfp";
}

void MergeStrategyFactoryDFP::add_options_to_parser(options::OptionParser &parser) {
    vector<string> atomic_ts_order;
    vector<string> atomic_ts_order_documentation;
    atomic_ts_order.push_back("regular");
    atomic_ts_order_documentation.push_back("the variable order of Fast Downward");
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

    parser.add_option<bool>(
        "randomized_order",
        "If true, use a 'globally' randomized order, i.e. all transition "
        "systems are considered in an arbitrary order. This renders all other "
        "ordering options void.",
        "false");
    utils::add_rng_options(parser);
}

static shared_ptr<MergeStrategyFactory>_parse(options::OptionParser &parser) {
    MergeStrategyFactoryDFP::add_options_to_parser(parser);

    options::Options options = parser.parse();
    parser.document_synopsis(
        "Merge strategy DFP",
        "This merge strategy implements the algorithm originally described in the "
        "paper \"Directed model checking with distance-preserving abstractions\" "
        "by Draeger, Finkbeiner and Podelski (SPIN 2006), adapted to planning in "
        "the following paper:" + utils::format_paper_reference(
            {"Silvan Sievers", "Martin Wehrle", "Malte Helmert"},
            "Generalized Label Reduction for Merge-and-Shrink Heuristics",
            "http://ai.cs.unibas.ch/papers/sievers-et-al-aaai2014.pdf",
            "Proceedings of the 28th AAAI Conference on Artificial"
            " Intelligence (AAAI 2014)",
            "2358-2366",
            "AAAI Press 2014"));
    if (parser.dry_run())
        return nullptr;
    else
        return make_shared<MergeStrategyFactoryDFP>(options);
}

static options::PluginShared<MergeStrategyFactory> _plugin("merge_dfp", _parse);
}
