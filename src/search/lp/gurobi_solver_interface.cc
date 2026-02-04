#include "gurobi_solver_interface.h"

#include "lp_solver.h"

#include "../utils/system.h"

#include <algorithm>
#include <iostream>
#include <numeric>

/*
    Core Gurobi functionality is provided in C API (gurobi_c.h).
    We implement the interface using the C API to avoid linking issues.

    The implementation is based on examples from https://www.gurobi.com/documentation/9.5/refman/c_api_overview.html

    We use the following functions from the Gurobi C API:
        - GRBsetdblattrarray:  sets the values of an array of double attributes. 
            - model: model object (GRBmodel *)
            - attr: attribute to set (const char *)
            - first: index of first element to set (int, zero-based)
            - len: number of elements to set (int)
            - values: a pointer to an array of double values (double *)
        - GRBsetdblattrelement: sets the value of a single element of a double attribute array.
            - model: model object (GRBmodel *)
            - attr: attribute to set (const char *)
            - element: index of element to set (int, zero-based)
            - newvalue: new value for the element (double)
    
    The following attributes are used:
        - GRB_DBL_ATTR_OBJ: is a double array attribute that contains the objective coefficients for all variables in the model. 
        - GRB_INT_ATTR_MODELSENSE: is an integer attribute that defines the optimization sense of the model. 
            - 1 for minimization
            - -1 for maximization
*/

using namespace std;

namespace lp {

namespace {
NO_RETURN void handle_gurobi_error(GRBenv *env, int error_code) {
    if (error_code == GRB_ERROR_OUT_OF_MEMORY) {
        utils::exit_with(utils::ExitCode::SEARCH_OUT_OF_MEMORY);
    }
    const char *msg = env ? GRBgeterrormsg(env) : nullptr;
    if (msg) {
        cerr << msg << endl;
    } else {
        cerr << "Gurobi error: code " << error_code << endl;
    }
    utils::exit_with(utils::ExitCode::SEARCH_CRITICAL_ERROR);
}

template<typename Func, typename... Args>
void GRB_CALL(GRBenv *env, Func fn, Args &&...args) {
    int status = fn(std::forward<Args>(args)...);
    if (status) {
        handle_gurobi_error(env, status);
    }
}

int objective_sense_to_gurobi(LPObjectiveSense sense) {
    if (sense == LPObjectiveSense::MINIMIZE) {
        return 1;
    } else {
        return -1;
    }
}

bool is_pos_infinity(double x) {
    return x >= GRB_INFINITY;
}

bool is_neg_infinity(double x) {
    return x <= -GRB_INFINITY;
}

void add_constraint(GRBenv *env, GRBmodel *model, const LPConstraint &constraint) {
    const vector<int> &indices = constraint.get_variables();
    const vector<double> &coefficients = constraint.get_coefficients();
    int numnz = static_cast<int>(indices.size());
    int *cind = numnz ? const_cast<int *>(indices.data()) : nullptr;
    double *cval = numnz ? const_cast<double *>(coefficients.data()) : nullptr;

    double lb = constraint.get_lower_bound();
    double ub = constraint.get_upper_bound();
    if (lb > ub) {
        // Add a trivially unsatisfiable row. 
        // TODO: should we handle this differently?
        GRB_CALL(env, GRBaddconstr, model, 0, nullptr, nullptr, GRB_LESS_EQUAL, -1.0, nullptr);
        return;
    }
    if (is_neg_infinity(lb) && is_pos_infinity(ub)) {
        // Constraint without bounds: do nothing.
        // TODO: is this the right behavior?
        return;
    }
    if (is_neg_infinity(lb)) {
        GRB_CALL(env, GRBaddconstr, model, numnz, cind, cval, GRB_LESS_EQUAL, ub, nullptr);
    } else if (is_pos_infinity(ub)) {
        GRB_CALL(env, GRBaddconstr, model, numnz, cind, cval, GRB_GREATER_EQUAL, lb, nullptr);
    } else if (lb == ub) {
        GRB_CALL(env, GRBaddconstr, model, numnz, cind, cval, GRB_EQUAL, lb, nullptr);
    } else {
        cerr << "Range constraints are not supported by Gurobi." << endl;
        utils::exit_with(utils::ExitCode::SEARCH_CRITICAL_ERROR);
    }
}

} // Have to add this otherwise the compiler complains.

GurobiSolverInterface::GurobiSolverInterface(): env(nullptr), model(nullptr), num_permanent_constraints(0), num_temporary_constraints(0), model_dirty(false) {
    GRB_CALL(env, GRBloadenvinternal, &env, "", GRB_VERSION_MAJOR, GRB_VERSION_MINOR, GRB_VERSION_TECHNICAL);
    GRB_CALL(env, GRBsetintparam, env, GRB_INT_PAR_OUTPUTFLAG, 0);
    GRB_CALL(env, GRBsetintparam, env, GRB_INT_PAR_LOGTOCONSOLE, 0);
    GRB_CALL(env, GRBsetintparam, env, GRB_INT_PAR_THREADS, 1);
    GRB_CALL(env, GRBsetintparam, env, GRB_INT_PAR_METHOD, GRB_METHOD_DUAL);
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
    int num_vars = variables.size();
    vector<double> obj;
    vector<double> lb;
    vector<double> ub;
    vector<char> vtype;
    obj.reserve(num_vars);
    lb.reserve(num_vars);
    ub.reserve(num_vars);
    vtype.reserve(num_vars);
    for (const LPVariable &var : variables) {
        obj.push_back(var.objective_coefficient);
        lb.push_back(var.lower_bound);
        ub.push_back(var.upper_bound);
        vtype.push_back(var.is_integer ? GRB_INTEGER : GRB_CONTINUOUS);
    }
    double *obj_ptr = num_vars ? obj.data() : nullptr;
    double *lb_ptr = num_vars ? lb.data() : nullptr;
    double *ub_ptr = num_vars ? ub.data() : nullptr;
    char *vtype_ptr = num_vars ? vtype.data() : nullptr;
    GRB_CALL(env, GRBnewmodel, env, &model, "downward", num_vars, obj_ptr, lb_ptr, ub_ptr, vtype_ptr, nullptr);
    GRB_CALL(env, GRBsetintattr, model, GRB_INT_ATTR_MODELSENSE, objective_sense_to_gurobi(lp.get_sense()));
    const auto &constraints = lp.get_constraints();
    for (const LPConstraint &constraint : constraints) {
        add_constraint(env, model, constraint);
    }
    //GRB_CALL(env, GRBupdatemodel, model);
    num_permanent_constraints = constraints.size();
    num_temporary_constraints = 0;
    model_dirty = true;
}

void GurobiSolverInterface::add_temporary_constraints(const named_vector::NamedVector<LPConstraint> &constraints) {
    for (const LPConstraint &constraint : constraints) {
        add_constraint(env, model, constraint);
    }
    //GRB_CALL(env, GRBupdatemodel, model);
    model_dirty = true;
    num_temporary_constraints += constraints.size();
}

void GurobiSolverInterface::clear_temporary_constraints() {
    if (!has_temporary_constraints()) {
        return;
    }
    vector<int> indices(num_temporary_constraints);
    iota(indices.begin(), indices.end(), num_permanent_constraints);
    GRB_CALL(env, GRBdelconstrs, model, num_temporary_constraints, indices.data());
    //GRB_CALL(env, GRBupdatemodel, model);
    model_dirty = true;
    num_temporary_constraints = 0;
}

double GurobiSolverInterface::get_infinity() const {
    return GRB_INFINITY;
}

void GurobiSolverInterface::set_objective_coefficients(const vector<double> &coefficients) {
    int num_coefficients = coefficients.size();
    if (!num_coefficients) {
        return;
    }                                                                                // TODO: is there a more elegant way to handle this?
    GRB_CALL(env, GRBsetdblattrarray, model, GRB_DBL_ATTR_OBJ, 0, num_coefficients, const_cast<double *>(coefficients.data()));
    //GRB_CALL(env, GRBupdatemodel, model);
    model_dirty = true;
}

void GurobiSolverInterface::set_objective_coefficient(int index, double coefficient) {
    GRB_CALL(env, GRBsetdblattrelement, model, GRB_DBL_ATTR_OBJ, index, coefficient);
    //GRB_CALL(env, GRBupdatemodel, model);
    model_dirty = true;
}

void GurobiSolverInterface::set_constraint_lower_bound(int index, double bound) {
    char sense;
    GRB_CALL(env, GRBgetcharattrelement, model, GRB_CHAR_ATTR_SENSE, index, &sense);
    if(sense == GRB_LESS_EQUAL) {
        if (!is_neg_infinity(bound)) {
            cerr << "Error: cannot set lower bound on <= constraint to a finite value." << endl;
            utils::exit_with(utils::ExitCode::SEARCH_CRITICAL_ERROR);
        }
        return;
    }
    else if(sense == GRB_GREATER_EQUAL) {  
        if (is_neg_infinity(bound)) {
            cerr << "Error: cannot set lower bound on >= constraint to -infinity." << endl;
            utils::exit_with(utils::ExitCode::SEARCH_CRITICAL_ERROR);
        } else {
            GRB_CALL(env, GRBsetdblattrelement, model, GRB_DBL_ATTR_RHS, index, bound);
            return;
        }
    }
    else if(sense == GRB_EQUAL) {
        double current_rhs;
        GRB_CALL(env, GRBgetdblattrelement, model, GRB_DBL_ATTR_RHS, index, &current_rhs);
        if (current_rhs != bound) {
            cerr << "Error: cannot change lower bound on = constraint." << endl;
            utils::exit_with(utils::ExitCode::SEARCH_CRITICAL_ERROR);
        }
        return;
    }
    //GRB_CALL(env, GRBupdatemodel, model);
    model_dirty = true;
}

void GurobiSolverInterface::set_constraint_upper_bound(int index, double bound) {
    char sense;
    GRB_CALL(env, GRBgetcharattrelement, model, GRB_CHAR_ATTR_SENSE, index, &sense);
    if(sense == GRB_LESS_EQUAL) {
        if (is_pos_infinity(bound)) {
            cerr << "Error: cannot set upper bound on <= constraint to +infinity." << endl;
            utils::exit_with(utils::ExitCode::SEARCH_CRITICAL_ERROR);
        } else {
            GRB_CALL(env, GRBsetdblattrelement, model, GRB_DBL_ATTR_RHS, index, bound);
            return;
        }
    }
    else if(sense == GRB_GREATER_EQUAL) {  
        if (!is_pos_infinity(bound)) {
            cerr << "Error: cannot set upper bound on >= constraint to a finite value." << endl;
            utils::exit_with(utils::ExitCode::SEARCH_CRITICAL_ERROR);
        }
        return;
    }
    else if(sense == GRB_EQUAL) {
        double current_rhs;
        GRB_CALL(env, GRBgetdblattrelement, model, GRB_DBL_ATTR_RHS, index, &current_rhs);
        if (current_rhs != bound) {
            cerr << "Error: cannot change upper bound on = constraint." << endl;
            utils::exit_with(utils::ExitCode::SEARCH_CRITICAL_ERROR);
        }
        return;
    }
    //GRB_CALL(env, GRBupdatemodel, model);
    model_dirty = true;
}

void GurobiSolverInterface::set_variable_lower_bound(int index, double bound) {
    GRB_CALL(env, GRBsetdblattrelement, model, GRB_DBL_ATTR_LB, index, bound);
    //GRB_CALL(env, GRBupdatemodel, model);
    model_dirty = true;
}

void GurobiSolverInterface::set_variable_upper_bound(int index, double bound) {
    GRB_CALL(env, GRBsetdblattrelement, model, GRB_DBL_ATTR_UB, index, bound);
    //GRB_CALL(env, GRBupdatemodel, model);
    model_dirty = true;
}

void GurobiSolverInterface::set_mip_gap(double gap) {
    GRB_CALL(env, GRBsetdblparam, env, GRB_DBL_PAR_MIPGAP, gap);
}

void GurobiSolverInterface::solve() {
    if (model_dirty) {
        GRB_CALL(env, GRBupdatemodel, model);
        model_dirty = false;
    }
    GRB_CALL(env, GRBoptimize, model);
}

void GurobiSolverInterface::write_lp(const string &filename) const {
    //if (model_dirty) {
    //    GRB_CALL(env, GRBupdatemodel, model);
    //    model_dirty = false;
    //}
    GRB_CALL(env, GRBwrite, model, filename.c_str());
}

// TODO: implement more detailed failure analysis
void GurobiSolverInterface::print_failure_analysis() const {
    int status = 0;
    GRB_CALL(env, GRBgetintattr, model, GRB_INT_ATTR_STATUS, &status);
    cout << "Gurobi status code: " << status << endl;
}

// TODO: check if a solution is available
bool GurobiSolverInterface::is_infeasible() const {
    int status = 0;
    GRB_CALL(env, GRBgetintattr, model, GRB_INT_ATTR_STATUS, &status);
    return status == GRB_INFEASIBLE;
}

// TODO: check if a solution is available
bool GurobiSolverInterface::is_unbounded() const {
    int status = 0;
    GRB_CALL(env, GRBgetintattr, model, GRB_INT_ATTR_STATUS, &status);
    return status == GRB_UNBOUNDED;
}

// TODO: check if a solution is available
bool GurobiSolverInterface::has_optimal_solution() const {
    int status = 0;
    GRB_CALL(env, GRBgetintattr, model, GRB_INT_ATTR_STATUS, &status);
    return status == GRB_OPTIMAL;
}

// TODO: check if a solution is available
double GurobiSolverInterface::get_objective_value() const {
    double value = 0.0;
    GRB_CALL(env, GRBgetdblattr, model, GRB_DBL_ATTR_OBJVAL, &value);
    return value;
}

// TODO: check if an optimal solution is available
vector<double> GurobiSolverInterface::extract_solution() const {
    int num_variables = get_num_variables();
    vector<double> solution(num_variables);
    if (num_variables > 0) {
        GRB_CALL(env, GRBgetdblattrarray, model, GRB_DBL_ATTR_X, 0, num_variables, solution.data());
    }
    return solution;
}

int GurobiSolverInterface::get_num_variables() const {
    int num_variables = 0;
    GRB_CALL(env, GRBgetintattr, model, GRB_INT_ATTR_NUMVARS, &num_variables);
    return num_variables;
}

int GurobiSolverInterface::get_num_constraints() const {
    return num_permanent_constraints + num_temporary_constraints;
}

bool GurobiSolverInterface::has_temporary_constraints() const {
    return num_temporary_constraints > 0;
}

void GurobiSolverInterface::print_statistics() const {
    double runtime = 0.0;
    GRB_CALL(env, GRBgetdblattr, model, GRB_DBL_ATTR_RUNTIME, &runtime);
    cout << "Gurobi runtime: " << runtime << "s" << endl;
}
}
