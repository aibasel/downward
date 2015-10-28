#include "lm_cut_constraints.h"

#include "../lm_cut_landmarks.h"
#include "../lp_solver.h"
#include "../option_parser.h"
#include "../plugin.h"
#include "../utilities.h"

#include <cassert>

using namespace std;

namespace OperatorCounting {

void LMCutConstraints::initialize_constraints(
    const shared_ptr<AbstractTask> task, vector<LPConstraint> &/*constraints*/,
    double /*infinity*/) {
    TaskProxy task_proxy(*task);
    landmark_generator = make_unique_ptr<LandmarkCutLandmarks>(task_proxy);
}


bool LMCutConstraints::update_constraints(const State &state,
                                          LPSolver &lp_solver) {
    assert(landmark_generator);
    vector<LPConstraint> constraints;
    double infinity = lp_solver.get_infinity();

    bool dead_end = landmark_generator->compute_landmarks(
        state, nullptr,
        [&](const vector<int> &op_ids, int /*cost*/) {
            constraints.emplace_back(1.0, infinity);
            LPConstraint &landmark_constraint = constraints.back();
            for (int op_id : op_ids) {
                landmark_constraint.insert(op_id, 1.0);
            }
        });

    if (dead_end) {
        return true;
    } else {
        lp_solver.add_temporary_constraints(constraints);
        return false;
    }
}

static shared_ptr<ConstraintGenerator> _parse(OptionParser &parser) {
    if (parser.dry_run())
        return nullptr;
    return make_shared<LMCutConstraints>();
}

static PluginShared<ConstraintGenerator> _plugin("lmcut_constraints", _parse);
}
