#include "gurobi_solver_interface.h"

#include "lp_solver.h"

#include "../utils/system.h"

#include <cassert>
#include <iostream>
#include <numeric>
#include <utility>

using namespace std;

namespace lp {

namespace {

NO_RETURN void handle_gurobi_error(GRBenv *env, int error_code) {
    if (error_code == GRB_ERROR_OUT_OF_MEMORY) {
        utils::exit_with(utils::ExitCode::SEARCH_OUT_OF_MEMORY);
    }

    const char *message = env ? GRBgeterrormsg(env) : nullptr;
    if (message) {
        cerr << "Gurobi error: " << message << endl;
    } else {
        cerr << "Gurobi error." << endl;
    }
    cerr << "Gurobi error code: " << error_code << endl;

    utils::exit_with(utils::ExitCode::SEARCH_CRITICAL_ERROR);
}

template<typename Function, typename... Args>
void GRB_CALL(GRBenv *env, Function function, Args &&...args) {
    int status = function(forward<Args>(args)...);
    if (status) {
        handle_gurobi_error(env, status);
    }
}

int objective_sense_to_gurobi(LPObjectiveSense sense) {
    switch (sense) {
    case LPObjectiveSense::MINIMIZE:
        return GRB_MINIMIZE;
    case LPObjectiveSense::MAXIMIZE:
        return GRB_MAXIMIZE;
    }

    ABORT("Unknown LP objective sense.");
}

char constraint_sense_to_gurobi(Sense sense) {
    switch (sense) {
    case Sense::GE:
        return GRB_GREATER_EQUAL;
    case Sense::LE:
        return GRB_LESS_EQUAL;
    case Sense::EQ:
        return GRB_EQUAL;
    }

    ABORT("Unknown LP constraint sense.");
}

void add_constraint(
    GRBenv *env, GRBmodel *model, const LPConstraint &constraint) {
    const vector<int> &indices = constraint.get_variables();
    const vector<double> &coefficients = constraint.get_coefficients();

    assert(indices.size() == coefficients.size());

    int num_nonzero = static_cast<int>(indices.size());
    int *index_data =
        num_nonzero > 0 ? const_cast<int *>(indices.data()) : nullptr;
    double *coefficient_data =
        num_nonzero > 0
            ? const_cast<double *>(coefficients.data())
            : nullptr;

    GRB_CALL(
        env,
        GRBaddconstr,
        model,
        num_nonzero,
        index_data,
        coefficient_data,
        constraint_sense_to_gurobi(constraint.get_sense()),
        constraint.get_right_hand_side(),
        nullptr);
}

int get_model_status(GRBenv *env, GRBmodel *model) {
    assert(model);

    int status;
    GRB_CALL(
        env,
        GRBgetintattr,
        model,
        GRB_INT_ATTR_STATUS,
        &status);
    return status;
}

} // namespace

GurobiSolverInterface::GurobiSolverInterface()
    : env(nullptr),
      model(nullptr),
      num_permanent_constraints(0),
      num_temporary_constraints(0),
      model_dirty(false) {
    int status = GRBloadenv(&env, "");
    if (status) {
        handle_gurobi_error(env, status);
    }

    GRB_CALL(
        env,
        GRBsetintparam,
        env,
        GRB_INT_PAR_OUTPUTFLAG,
        0);
    GRB_CALL(
        env,
        GRBsetintparam,
        env,
        GRB_INT_PAR_LOGTOCONSOLE,
        0);
    GRB_CALL(
        env,
        GRBsetintparam,
        env,
        GRB_INT_PAR_THREADS,
        1);
    GRB_CALL(
        env,
        GRBsetintparam,
        env,
        GRB_INT_PAR_METHOD,
        GRB_METHOD_DUAL);
}

GurobiSolverInterface::~GurobiSolverInterface() {
    if (model) {
        GRBfreemodel(model);
    }
    if (env) {
        GRBfreeenv(env);
    }
}

void GurobiSolverInterface::load_problem(const LinearProgram &lp) {
    if (model) {
        GRBfreemodel(model);
        model = nullptr;
    }

    const auto &variables = lp.get_variables();
    int num_variables = static_cast<int>(variables.size());

    vector<double> objective_coefficients;
    vector<double> lower_bounds;
    vector<double> upper_bounds;
    vector<char> variable_types;

    objective_coefficients.reserve(num_variables);
    lower_bounds.reserve(num_variables);
    upper_bounds.reserve(num_variables);
    variable_types.reserve(num_variables);

    for (const LPVariable &variable : variables) {
        objective_coefficients.push_back(
            variable.objective_coefficient);
        lower_bounds.push_back(variable.lower_bound);
        upper_bounds.push_back(variable.upper_bound);
        variable_types.push_back(
            variable.is_integer ? GRB_INTEGER : GRB_CONTINUOUS);
    }

    double *objective_data =
        num_variables > 0 ? objective_coefficients.data() : nullptr;
    double *lower_bound_data =
        num_variables > 0 ? lower_bounds.data() : nullptr;
    double *upper_bound_data =
        num_variables > 0 ? upper_bounds.data() : nullptr;
    char *variable_type_data =
        num_variables > 0 ? variable_types.data() : nullptr;

    GRB_CALL(
        env,
        GRBnewmodel,
        env,
        &model,
        "downward",
        num_variables,
        objective_data,
        lower_bound_data,
        upper_bound_data,
        variable_type_data,
        nullptr);

    GRB_CALL(
        env,
        GRBsetintattr,
        model,
        GRB_INT_ATTR_MODELSENSE,
        objective_sense_to_gurobi(lp.get_sense()));

    const auto &constraints = lp.get_constraints();

    num_permanent_constraints =
        static_cast<int>(constraints.size());
    num_temporary_constraints = 0;

    for (const LPConstraint &constraint : constraints) {
        add_constraint(env, model, constraint);
    }

    GRB_CALL(env, GRBupdatemodel, model);
    model_dirty = false;
}

void GurobiSolverInterface::add_temporary_constraints(
    const named_vector::NamedVector<LPConstraint> &constraints) {
    assert(model);

    for (const LPConstraint &constraint : constraints) {
        add_constraint(env, model, constraint);
    }

    num_temporary_constraints +=
        static_cast<int>(constraints.size());
    model_dirty = true;
}

void GurobiSolverInterface::clear_temporary_constraints() {
    assert(model);

    if (!has_temporary_constraints()) {
        return;
    }

    if (model_dirty) {
        GRB_CALL(env, GRBupdatemodel, model);
        model_dirty = false;
    }

    vector<int> indices(num_temporary_constraints);
    iota(
        indices.begin(),
        indices.end(),
        num_permanent_constraints);

    GRB_CALL(
        env,
        GRBdelconstrs,
        model,
        num_temporary_constraints,
        indices.data());

    GRB_CALL(env, GRBupdatemodel, model);

    num_temporary_constraints = 0;
    model_dirty = false;
}

double GurobiSolverInterface::get_infinity() const {
    return GRB_INFINITY;
}

void GurobiSolverInterface::set_objective_coefficients(
    const vector<double> &coefficients) {
    assert(model);
    assert(
        coefficients.size() ==
        static_cast<size_t>(get_num_variables()));

    if (coefficients.empty()) {
        return;
    }

    GRB_CALL(
        env,
        GRBsetdblattrarray,
        model,
        GRB_DBL_ATTR_OBJ,
        0,
        static_cast<int>(coefficients.size()),
        const_cast<double *>(coefficients.data()));

    model_dirty = true;
}

void GurobiSolverInterface::set_objective_coefficient(
    int index, double coefficient) {
    assert(model);
    assert(index >= 0 && index < get_num_variables());

    GRB_CALL(
        env,
        GRBsetdblattrelement,
        model,
        GRB_DBL_ATTR_OBJ,
        index,
        coefficient);

    model_dirty = true;
}

void GurobiSolverInterface::set_constraint_rhs(
    int index, double right_hand_side) {
    assert(model);
    assert(index >= 0 && index < get_num_constraints());

    if (model_dirty) {
        GRB_CALL(env, GRBupdatemodel, model);
        model_dirty = false;
    }

    GRB_CALL(
        env,
        GRBsetdblattrelement,
        model,
        GRB_DBL_ATTR_RHS,
        index,
        right_hand_side);

    model_dirty = true;
}

void GurobiSolverInterface::set_constraint_sense(
    int index, Sense sense) {
    assert(model);
    assert(index >= 0 && index < get_num_constraints());

    if (model_dirty) {
        GRB_CALL(env, GRBupdatemodel, model);
        model_dirty = false;
    }

    GRB_CALL(
        env,
        GRBsetcharattrelement,
        model,
        GRB_CHAR_ATTR_SENSE,
        index,
        constraint_sense_to_gurobi(sense));

    model_dirty = true;
}

void GurobiSolverInterface::set_variable_lower_bound(
    int index, double bound) {
    assert(model);
    assert(index >= 0 && index < get_num_variables());

    GRB_CALL(
        env,
        GRBsetdblattrelement,
        model,
        GRB_DBL_ATTR_LB,
        index,
        bound);

    model_dirty = true;
}

void GurobiSolverInterface::set_variable_upper_bound(
    int index, double bound) {
    assert(model);
    assert(index >= 0 && index < get_num_variables());

    GRB_CALL(
        env,
        GRBsetdblattrelement,
        model,
        GRB_DBL_ATTR_UB,
        index,
        bound);

    model_dirty = true;
}

void GurobiSolverInterface::set_mip_gap(double gap) {
    assert(gap >= 0.0);

    GRB_CALL(
        env,
        GRBsetdblparam,
        env,
        GRB_DBL_PAR_MIPGAP,
        gap);

    if (model) {
        GRBenv *model_env = GRBgetenv(model);
        GRB_CALL(
            model_env,
            GRBsetdblparam,
            model_env,
            GRB_DBL_PAR_MIPGAP,
            gap);
    }
}

void GurobiSolverInterface::solve() {
    assert(model);

    if (model_dirty) {
        GRB_CALL(env, GRBupdatemodel, model);
        model_dirty = false;
    }

    GRB_CALL(env, GRBoptimize, model);
}

void GurobiSolverInterface::write_lp(
    const string &filename) const {
    assert(model);

    if (model_dirty) {
        GRB_CALL(env, GRBupdatemodel, model);
    }

    GRB_CALL(env, GRBwrite, model, filename.c_str());
}

void GurobiSolverInterface::print_failure_analysis() const {
    assert(model);

    int status = get_model_status(env, model);

    int solution_count;
    GRB_CALL(
        env,
        GRBgetintattr,
        model,
        GRB_INT_ATTR_SOLCOUNT,
        &solution_count);

    cerr << "Gurobi optimization failed with status "
         << status << " and " << solution_count
         << " stored solution(s)." << endl;

    switch (status) {
    case GRB_INFEASIBLE:
        cerr << "The model is infeasible." << endl;
        break;
    case GRB_UNBOUNDED:
        cerr << "The model is unbounded." << endl;
        break;
    case GRB_INF_OR_UNBD:
        cerr << "The model is infeasible or unbounded." << endl;
        break;
    case GRB_TIME_LIMIT:
        cerr << "The time limit was reached." << endl;
        break;
    case GRB_ITERATION_LIMIT:
        cerr << "The iteration limit was reached." << endl;
        break;
    case GRB_NUMERIC:
        cerr << "Gurobi encountered numerical difficulties." << endl;
        break;
    case GRB_SUBOPTIMAL:
        cerr << "Gurobi found a suboptimal solution." << endl;
        break;
    case GRB_INTERRUPTED:
        cerr << "The optimization was interrupted." << endl;
        break;
    default:
        break;
    }
}

bool GurobiSolverInterface::is_infeasible() const {
    return get_model_status(env, model) == GRB_INFEASIBLE;
}

bool GurobiSolverInterface::is_unbounded() const {
    return get_model_status(env, model) == GRB_UNBOUNDED;
}

bool GurobiSolverInterface::has_optimal_solution() const {
    return get_model_status(env, model) == GRB_OPTIMAL;
}

double GurobiSolverInterface::get_objective_value() const {
    assert(model);
    assert(has_optimal_solution());

    double objective_value;
    GRB_CALL(
        env,
        GRBgetdblattr,
        model,
        GRB_DBL_ATTR_OBJVAL,
        &objective_value);

    return objective_value;
}

vector<double> GurobiSolverInterface::extract_solution() const {
    assert(model);
    assert(has_optimal_solution());

    int num_variables = get_num_variables();
    vector<double> solution(num_variables);

    if (num_variables > 0) {
        GRB_CALL(
            env,
            GRBgetdblattrarray,
            model,
            GRB_DBL_ATTR_X,
            0,
            num_variables,
            solution.data());
    }

    return solution;
}

int GurobiSolverInterface::get_num_variables() const {
    assert(model);

    int num_variables;
    GRB_CALL(
        env,
        GRBgetintattr,
        model,
        GRB_INT_ATTR_NUMVARS,
        &num_variables);

    return num_variables;
}

int GurobiSolverInterface::get_num_constraints() const {
    return num_permanent_constraints + num_temporary_constraints;
}

bool GurobiSolverInterface::has_temporary_constraints() const {
    return num_temporary_constraints > 0;
}

void GurobiSolverInterface::print_statistics() const {
    assert(model);

    double runtime;
    GRB_CALL(
        env,
        GRBgetdblattr,
        model,
        GRB_DBL_ATTR_RUNTIME,
        &runtime);

    cout << "Gurobi runtime: " << runtime << "s" << endl;
}

} // namespace lp
