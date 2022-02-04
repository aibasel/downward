#include "limited_pruning.h"

#include "../option_parser.h"
#include "../plugin.h"

#include "../utils/logging.h"

using namespace std;

namespace limited_pruning {
LimitedPruning::LimitedPruning(const Options &opts)
    : pruning_method(opts.get<shared_ptr<PruningMethod>>("pruning")),
      min_required_pruning_ratio(opts.get<double>("min_required_pruning_ratio")),
      num_expansions_before_checking_pruning_ratio(
          opts.get<int>("expansions_before_checking_pruning_ratio")),
      num_pruning_calls(0),
      is_pruning_disabled(false) {
}
void LimitedPruning::initialize(const shared_ptr<AbstractTask> &task) {
    PruningMethod::initialize(task);
    pruning_method->initialize(task);
    utils::g_log << "pruning method: limited" << endl;
}

void LimitedPruning::prune_operators(
    const State &state, std::vector<OperatorID> &op_ids) {
    if (is_pruning_disabled) {
        return;
    }
    if (min_required_pruning_ratio > 0. &&
        num_pruning_calls == num_expansions_before_checking_pruning_ratio) {
        double pruning_ratio = (num_unpruned_successors_generated == 0) ? 1. : 1. - (
            static_cast<double>(num_pruned_successors_generated) /
            static_cast<double>(num_unpruned_successors_generated));
        utils::g_log << "Pruning ratio after " << num_expansions_before_checking_pruning_ratio
                     << " calls: " << pruning_ratio << endl;
        if (pruning_ratio < min_required_pruning_ratio) {
            utils::g_log << "-- pruning ratio is lower than minimum pruning ratio ("
                         << min_required_pruning_ratio << ") -> switching off pruning" << endl;
            is_pruning_disabled = true;
        }
    }

    ++num_pruning_calls;
    pruning_method->prune_operators(state, op_ids);
}

static shared_ptr<PruningMethod> _parse(OptionParser &parser) {
    parser.document_synopsis(
        "Limited pruning",
        "Limited pruning applies another pruning method and switches it off "
        "after a fixed number of expansions if the pruning ratio is below a "
        "given value. The pruning ratio is defined as 1 - (A/B), where B and A "
        "are the total numbers of operators before and after pruning summed "
        "over all previous expansions.");
    parser.add_option<shared_ptr<PruningMethod>>("pruning");
    parser.add_option<double>(
        "min_required_pruning_ratio",
        "disable pruning if the pruning ratio is lower than this value after"
        " 'expansions_before_checking_pruning_ratio' expansions",
        "0.2",
        Bounds("0.0", "1.0"));
    parser.add_option<int>(
        "expansions_before_checking_pruning_ratio",
        "number of expansions before deciding whether to disable pruning",
        "1000",
        Bounds("0", "infinity"));

    Options opts = parser.parse();
    if (parser.dry_run()) {
        return nullptr;
    }

    return make_shared<LimitedPruning>(opts);
}

static Plugin<PruningMethod> _plugin("limited", _parse);
}
