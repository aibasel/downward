#include "limited_pruning.h"

#include "../plugins/plugin.h"
#include "../utils/logging.h"

using namespace std;

namespace limited_pruning {
LimitedPruning::LimitedPruning(
    const shared_ptr<PruningMethod> &pruning,
    double min_required_pruning_ratio,
    int expansions_before_checking_pruning_ratio,
    utils::Verbosity verbosity)
    : PruningMethod(verbosity),
      pruning_method(pruning),
      min_required_pruning_ratio(min_required_pruning_ratio),
      num_expansions_before_checking_pruning_ratio(
          expansions_before_checking_pruning_ratio),
      num_pruning_calls(0),
      is_pruning_disabled(false) {
}

void LimitedPruning::initialize(const shared_ptr<AbstractTask> &task) {
    PruningMethod::initialize(task);
    pruning_method->initialize(task);
    log << "pruning method: limited" << endl;
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
        if (log.is_at_least_normal()) {
            log << "Pruning ratio after " << num_expansions_before_checking_pruning_ratio
                << " calls: " << pruning_ratio << endl;
        }
        if (pruning_ratio < min_required_pruning_ratio) {
            if (log.is_at_least_normal()) {
                log << "-- pruning ratio is lower than minimum pruning ratio ("
                    << min_required_pruning_ratio << ") -> switching off pruning" << endl;
            }
            is_pruning_disabled = true;
        }
    }

    ++num_pruning_calls;
    pruning_method->prune(state, op_ids);
}

class LimitedPruningFeature
    : public plugins::TypedFeature<PruningMethod, LimitedPruning> {
public:
    LimitedPruningFeature() : TypedFeature("limited_pruning") {
        document_title("Limited pruning");
        document_synopsis(
            "Limited pruning applies another pruning method and switches it off "
            "after a fixed number of expansions if the pruning ratio is below a "
            "given value. The pruning ratio is the sum of all pruned operators "
            "divided by the sum of all operators before pruning, considering all "
            "previous expansions.");

        add_option<shared_ptr<PruningMethod>>(
            "pruning",
            "the underlying pruning method to be applied");
        add_option<double>(
            "min_required_pruning_ratio",
            "disable pruning if the pruning ratio is lower than this value after"
            " 'expansions_before_checking_pruning_ratio' expansions",
            "0.2",
            plugins::Bounds("0.0", "1.0"));
        add_option<int>(
            "expansions_before_checking_pruning_ratio",
            "number of expansions before deciding whether to disable pruning",
            "1000",
            plugins::Bounds("0", "infinity"));
        add_pruning_options_to_feature(*this);

        document_note(
            "Example",
            "To use atom centric stubborn sets and limit them, use\n"
            "{{{\npruning=limited_pruning(pruning=atom_centric_stubborn_sets(),"
            "min_required_pruning_ratio=0.2,expansions_before_checking_pruning_ratio=1000)\n}}}\n"
            "in an eager search such as astar.");
    }

    virtual shared_ptr<LimitedPruning> create_component(
        const plugins::Options &opts,
        const utils::Context &) const override {
        return plugins::make_shared_from_arg_tuples<LimitedPruning>(
            opts.get<shared_ptr<PruningMethod>>("pruning"),
            opts.get<double>("min_required_pruning_ratio"),
            opts.get<int>("expansions_before_checking_pruning_ratio"),
            get_pruning_arguments_from_options(opts));
    }
};

static plugins::FeaturePlugin<LimitedPruningFeature> _plugin;
}
