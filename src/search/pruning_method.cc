#include "pruning_method.h"

#include "plugins/plugin.h"
#include "task_utils/task_properties.h"

#include "utils/logging.h"

#include <cassert>

using namespace std;

PruningMethod::PruningMethod(const plugins::Options &opts)
    : timer(false),
      log(utils::get_log_from_options(opts)),
      task(nullptr) {
}

void PruningMethod::initialize(const shared_ptr<AbstractTask> &task_) {
    assert(!task);
    task = task_;
    num_successors_before_pruning = 0;
    num_successors_after_pruning = 0;
}

void PruningMethod::prune_operators(
    const State &state, vector<OperatorID> &op_ids) {
    assert(!task_properties::is_goal_state(TaskProxy(*task), state));
    /*
      We only measure time with verbose verbosity level because time
      measurements induce a significant overhead, up to 30% for configurations
      like blind search. See issue1042 and issue1058.
    */
    if (log.is_at_least_verbose()) {
        timer.resume();
    }
    int num_ops_before_pruning = op_ids.size();
    prune(state, op_ids);
    num_successors_before_pruning += num_ops_before_pruning;
    num_successors_after_pruning += op_ids.size();
    if (log.is_at_least_verbose()) {
        timer.stop();
    }
}

void PruningMethod::print_statistics() const {
    if (log.is_at_least_normal()) {
        log << "total successors before pruning: "
            << num_successors_before_pruning << endl
            << "total successors after pruning: "
            << num_successors_after_pruning << endl;
        double pruning_ratio = (num_successors_before_pruning == 0) ? 1. : 1. - (
            static_cast<double>(num_successors_after_pruning) /
            static_cast<double>(num_successors_before_pruning));
        log << "Pruning ratio: " << pruning_ratio << endl;
        if (log.is_at_least_verbose()) {
            log << "Time for pruning operators: " << timer << endl;
        }
    }
}

void add_pruning_options_to_feature(plugins::Feature &feature) {
    utils::add_log_options_to_feature(feature);
    feature.document_note(
        "Note on verbosity parameter",
        "Setting verbosity to verbose or higher enables time measurements "
        "in each call to prune_operators for a given state. This induces a "
        "significant overhead, up to 30% in configurations like blind search "
        "with the no pruning method (`null`). We recommend using at most "
        "normal verbosity for running experiments.");
}

static class PruningMethodCategoryPlugin : public plugins::TypedCategoryPlugin<PruningMethod> {
public:
    PruningMethodCategoryPlugin() : TypedCategoryPlugin("PruningMethod") {
        document_synopsis("Prune or reorder applicable operators.");
    }
}
_category_plugin;
