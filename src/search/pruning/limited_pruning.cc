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

void LimitedPruning::prune(
    const State &state, vector<OperatorID> &op_ids) {
    if (is_pruning_disabled) {
        return;
    }
    if (num_pruning_calls == num_expansions_before_checking_pruning_ratio &&
        min_required_pruning_ratio > 0.) {
        double pruning_ratio = (num_successors_before_pruning == 0) ? 1. : 1. - (
            static_cast<double>(num_successors_after_pruning) /
            static_cast<double>(num_successors_before_pruning));
        utils::g_log << "Pruning ratio after " << num_expansions_before_checking_pruning_ratio
                     << " calls: " << pruning_ratio << endl;
        if (pruning_ratio < min_required_pruning_ratio) {
            utils::g_log << "-- pruning ratio is lower than minimum pruning ratio ("
                         << min_required_pruning_ratio << ") -> switching off pruning" << endl;
            is_pruning_disabled = true;
        }
    }

    ++num_pruning_calls;
    pruning_method->prune(state, op_ids);
}

static shared_ptr<PruningMethod> _parse(OptionParser &parser) {
    parser.document_synopsis(
        "Limited pruning",
        "Limited pruning applies another pruning method and switches it off "
        "after a fixed number of expansions if the pruning ratio is below a "
        "given value. The pruning ratio is the sum of all pruned operators "
        "divided by the sum of all operators before pruning, considering all "
        "previous expansions.");
    parser.add_option<shared_ptr<PruningMethod>>(
        "pruning",
        "the underlying pruning method to be applied");
    parser.document_note(
        "Example",
        "To use atom centric stubborn sets and limit them, use\n"
        "{{{\npruning=limited_pruning(pruning=atom_centric_stubborn_sets(),"
        "min_required_pruning_ratio=0.2,expansions_before_checking_pruning_ratio=1000)\n}}}\n"
        "in an eager search such as astar.");
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

static Plugin<PruningMethod> _plugin("limited_pruning", _parse);
}
