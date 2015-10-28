#include "operator_counting_heuristic.h"

#include "constraint_generator.h"

#include "../option_parser.h"
#include "../plugin.h"

using namespace std;

namespace operator_counting {
OperatorCountingHeuristic::OperatorCountingHeuristic(const Options &opts)
    : Heuristic(opts),
      constraint_generators(
          opts.get_list<shared_ptr<ConstraintGenerator>>("constraint_generators")),
      lp_solver(LPSolverType(opts.get_enum("lpsolver"))) {
}

OperatorCountingHeuristic::~OperatorCountingHeuristic() {
}

void OperatorCountingHeuristic::initialize() {
    vector<LPVariable> variables;
    double infinity = lp_solver.get_infinity();
    for (OperatorProxy op : task_proxy.get_operators()) {
        int op_cost = op.get_cost();
        variables.push_back(LPVariable(0, infinity, op_cost));
    }
    vector<LPConstraint> constraints;
    for (auto generator : constraint_generators) {
        generator->initialize_constraints(task, constraints, infinity);
    }
    lp_solver.load_problem(LPObjectiveSense::MINIMIZE, variables, constraints);
}

int OperatorCountingHeuristic::compute_heuristic(const GlobalState &global_state) {
    State state = convert_global_state(global_state);
    return compute_heuristic(state);
}

int OperatorCountingHeuristic::compute_heuristic(const State &state) {
    assert(!lp_solver.has_temporary_constraints());
    for (auto generator : constraint_generators) {
        bool dead_end = generator->update_constraints(state, lp_solver);
        if (dead_end) {
            lp_solver.clear_temporary_constraints();
            return DEAD_END;
        }
    }
    int result;
    lp_solver.solve();
    if (lp_solver.has_optimal_solution()) {
        result = lp_solver.get_objective_value();
    } else {
        result = DEAD_END;
    }
    lp_solver.clear_temporary_constraints();
    return result;
}

static Heuristic *_parse(OptionParser &parser) {
    parser.add_list_option<ConstraintGenerator *>(
        "constraint_generators",
        "methods that generate constraints over LP variables "
        "representing the number of operator applications");
    add_lp_solver_option_to_parser(parser);
    Heuristic::add_options_to_parser(parser);
    Options opts = parser.parse();
    if (parser.help_mode())
        return nullptr;
    opts.verify_list_non_empty<ConstraintGenerator *>("constraint_generators");
    if (parser.dry_run())
        return nullptr;
    return new OperatorCountingHeuristic(opts);
}

static Plugin<Heuristic> _plugin("operatorcounting", _parse);
}
