#include "landmark_factory_relaxation.h"

#include "exploration.h"
#include "landmark.h"

#include "../task_utils/task_properties.h"

using namespace std;

namespace landmarks {
LandmarkFactoryRelaxation::LandmarkFactoryRelaxation(
    utils::Verbosity verbosity)
    : LandmarkFactory(verbosity) {
}

void LandmarkFactoryRelaxation::generate_landmarks(const shared_ptr<AbstractTask> &task) {
    TaskProxy task_proxy(*task);
    Exploration exploration(task_proxy, log);
    generate_relaxed_landmarks(task, exploration);
    postprocess(task_proxy, exploration);
}

void LandmarkFactoryRelaxation::postprocess(
    const TaskProxy &task_proxy, Exploration &exploration) {
    landmark_graph->set_landmark_ids();
    calc_achievers(task_proxy, exploration);
}

void LandmarkFactoryRelaxation::calc_achievers(
    const TaskProxy &task_proxy, Exploration &exploration) {
    assert(!achievers_calculated);
    VariablesProxy variables = task_proxy.get_variables();
    for (const auto &node : *landmark_graph) {
        Landmark &landmark = node->get_landmark();
        for (const FactPair &atom : landmark.atoms) {
            const vector<int> &ops = get_operators_including_effect(atom);
            landmark.possible_achievers.insert(ops.begin(), ops.end());

            if (variables[atom.var].is_derived())
                landmark.is_derived = true;
        }

        vector<vector<bool>> reached =
            exploration.compute_relaxed_reachability(landmark.atoms, false);

        for (int op_or_axom_id : landmark.possible_achievers) {
            OperatorProxy op = get_operator_or_axiom(task_proxy, op_or_axom_id);

            if (possibly_reaches_landmark(op, reached, landmark)) {
                landmark.first_achievers.insert(op_or_axom_id);
            }
        }
    }
    achievers_calculated = true;
}

// TODO: Move this to lm_exhaust and make it a static function.
bool LandmarkFactoryRelaxation::relaxed_task_solvable(
    const TaskProxy &task_proxy, Exploration &exploration,
    const Landmark &landmark, const bool use_unary_relaxation) {
    vector<vector<bool>> reached = exploration.compute_relaxed_reachability(
        landmark.atoms, use_unary_relaxation);

    for (FactProxy goal : task_proxy.get_goals()) {
        if (!reached[goal.get_variable().get_id()][goal.get_value()]) {
            return false;
        }
    }
    return true;
}
}
