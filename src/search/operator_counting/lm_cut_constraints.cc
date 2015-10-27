#include "lm_cut_constraints.h"

#include "../lm_cut_landmarks.h"
#include "../lp_solver.h"
#include "../option_parser.h"
#include "../plugin.h"
#include "../utilities.h"

#include <cassert>

using namespace std;

namespace operator_counting {
void LMCutConstraints::initialize_constraints(
    const TaskProxy &task_proxy, vector<LPConstraint> &constraints,
    double infinity) {
    unused_parameter(constraints);
    unused_parameter(infinity);
    landmark_generator = make_unique_ptr<LandmarkCutLandmarks>(task_proxy);
}


bool LMCutConstraints::update_constraints(const State &state,
                                          LPSolver &lp_solver) {
    assert(landmark_generator);
    vector<LPConstraint> constraints;
    double infinity = lp_solver.get_infinity();

    bool dead_end = landmark_generator->compute_landmarks(
        state, nullptr,
        [&](const vector<int> &op_ids, int cost) {
            unused_parameter(cost);
            constraints.emplace_back(1.0, infinity);
            LPConstraint &landmark_constraint = constraints.back();
            for (int op_id : op_ids) {
                landmark_constraint.insert(op_id, 1.0);
            }
        }
   );

    if (dead_end) {
        return true;
    } else {
        lp_solver.add_temporary_constraints(constraints);
        return false;
    }
}

static ConstraintGenerator *_parse(OptionParser &parser) {
    if (parser.dry_run())
        return 0;
    return new LMCutConstraints();
}

static Plugin<ConstraintGenerator> _plugin("lmcut_constraints", _parse);
}

