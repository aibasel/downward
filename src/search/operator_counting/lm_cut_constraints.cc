#include "lm_cut_constraints.h"

#include "../option_parser.h"
#include "../plugin.h"

#include "../heuristics/lm_cut_landmarks.h"

#include "../lp/lp_solver.h"

#include "../utils/memory.h"

#include <cassert>

using namespace std;

namespace operator_counting {
void LMCutConstraints::initialize_constraints(
    const shared_ptr<AbstractTask> task, vector<lp::LPConstraint> & /*constraints*/,
    double /*infinity*/) {
    TaskProxy task_proxy(*task);
    landmark_generator =
        utils::make_unique_ptr<lm_cut_heuristic::LandmarkCutLandmarks>(task_proxy);
}


bool LMCutConstraints::update_constraints(const State &state,
                                          lp::LPSolver &lp_solver) {
    assert(landmark_generator);
    vector<lp::LPConstraint> constraints;
    double infinity = lp_solver.get_infinity();

    bool dead_end = landmark_generator->compute_landmarks(
        state, nullptr,
        [&](const vector<int> &op_ids, int /*cost*/) {
            constraints.emplace_back(1.0, infinity);
            lp::LPConstraint &landmark_constraint = constraints.back();
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
    parser.document_synopsis(
        "LM-cut landmark constraints",
        "Computes a set of landmarks in each state using the LM-cut method. "
        "For each landmark L the constraint sum_{o in L} Count_o >= 1 is added "
        "to the operator counting LP temporarily. After the heuristic value "
        "for the state is computed, all temporary constraints are removed "
        "again. For details, see\n"
        " * Florian Pommerening, Gabriele Roeger, Malte Helmert and "
        "Blai Bonet.<<BR>>\n"
        " [LP-based Heuristics for Cost-optimal Planning "
        "http://www.aaai.org/ocs/index.php/ICAPS/ICAPS14/paper/view/7892/8031]."
        "<<BR>>\n "
        "In //Proceedings of the Twenty-Fourth International "
        "Conference on Automated Planning and Scheduling (ICAPS "
        "2014)//, pp. 226-234. AAAI Press 2014.\n"
        " * Blai Bonet.<<BR>>\n"
        " [An admissible heuristic for SAS+ planning obtained from the "
        "state equation "
        "http://ijcai.org/papers13/Papers/IJCAI13-335.pdf]."
        "<<BR>>\n "
        "In //Proceedings of the Twenty-Third International Joint "
        "Conference on Artificial Intelligence (IJCAI 2013)//, "
        "pp. 2268-2274. 2013.\n\n\n");

    if (parser.dry_run())
        return nullptr;
    return make_shared<LMCutConstraints>();
}

static PluginShared<ConstraintGenerator> _plugin("lmcut_constraints", _parse);
}
