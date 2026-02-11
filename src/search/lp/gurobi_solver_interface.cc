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
        cerr << "Gurobi error code: " << error_code << endl;
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

void add_constraint(GRBenv *env, GRBmodel *model, const LPConstraint &constraint) {
    const vector<int> &indices = constraint.get_variables();
    const vector<double> &coefficients = constraint.get_coefficients();
    int numnz = static_cast<int>(indices.size());
    int *cind = numnz ? const_cast<int *>(indices.data()) : nullptr;
    double *cval = numnz ? const_cast<double *>(coefficients.data()) : nullptr;

    double rhs = constraint.get_right_hand_side();
    lp::Sense sense = constraint.get_sense();

    //cerr << "Adding constraint with sense " << (sense == lp::Sense::GE ? "GE" : (sense == lp::Sense::LE ? "LE" : (sense == lp::Sense::EQ ? "EQ" : "UNKNOWN"))) << " and right-hand side " << rhs << endl;
    if (sense == lp::Sense::GE) {
        GRB_CALL(env, GRBaddconstr, model, numnz, cind, cval, GRB_GREATER_EQUAL, rhs, nullptr);
    } else if (sense == lp::Sense::LE) {
        GRB_CALL(env, GRBaddconstr, model, numnz, cind, cval, GRB_LESS_EQUAL, rhs, nullptr);
    } else if (sense == lp::Sense::EQ) {
        GRB_CALL(env, GRBaddconstr, model, numnz, cind, cval, GRB_EQUAL, rhs, nullptr);
    } else {
        cerr << "Error: Unknown constraint sense." << endl;
        utils::exit_with(utils::ExitCode::SEARCH_CRITICAL_ERROR);
    }

}
} // Have to add this otherwise the compiler complains.

GurobiSolverInterface::GurobiSolverInterface(): env(nullptr), model(nullptr), num_permanent_constraints(0), num_temporary_constraints(0), model_dirty(false) {
    //GRB_CALL(env, GRBloadenv, &env, "");
    GRBloadenv(&env, "");
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
    num_permanent_constraints = 0;
    num_temporary_constraints = 0;
    for (const LPConstraint &constraint : constraints) {
        add_constraint(env, model, constraint);
        num_permanent_constraints++;
    }
    GRB_CALL(env, GRBupdatemodel, model);
    model_dirty = false;

        // Print model
    //cerr << "Model loaded with " << num_vars << " variables and " << constraints.size() << " constraints." << endl;
    //for (int i = 0; i < num_vars; ++i) {
    //    cerr << "Variable " << i << ": obj=" << variables[i].objective_coefficient
    //         << ", lb=" << variables[i].lower_bound
    //         << ", ub=" << variables[i].upper_bound
    //         << ", is_integer=" << variables[i].is_integer
    //         << endl;
    //}
    //for (int i = 0; i < constraints.size(); ++i) {
    //    const auto &c = constraints[i];
    //    cerr << "Constraint " << i << ": sense=" 
    //         << (c.get_sense() == lp::Sense::GE ? "GE" : (c.get_sense() == lp::Sense::LE ? "LE" : (c.get_sense() == lp::Sense::EQ ? "EQ" : "UNKNOWN"))) 
    //         << ", rhs=" << c.get_right_hand_side()
    //         << ", coefficients=[";
    //    for (size_t j = 0; j < c.get_variables().size(); ++j) {
    //        cerr << "(" << c.get_variables()[j] << ": " << c.get_coefficients()[j] << ")";
    //        if (j + 1 < c.get_variables().size()) {
    //            cerr << ", ";
    //        }
    //    }
    //    cerr << "]" << endl;
    //}
}

void GurobiSolverInterface::add_temporary_constraints(const named_vector::NamedVector<LPConstraint> &constraints) {
    for (const LPConstraint &constraint : constraints) {
        add_constraint(env, model, constraint);
    }
    model_dirty = true;
    num_temporary_constraints += constraints.size();
    //cerr << ">>>>> Added " << constraints.size() << " temporary constraints. Total temporary constraints: " << num_temporary_constraints << endl;
}

void GurobiSolverInterface::clear_temporary_constraints() {
    if (!has_temporary_constraints()) {
        return;
    }
    vector<int> indices(num_temporary_constraints);
    iota(indices.begin(), indices.end(), num_permanent_constraints);
    GRB_CALL(env, GRBdelconstrs, model, num_temporary_constraints, indices.data());
    model_dirty = true;
    num_temporary_constraints = 0;
    //cerr << ">>>>> Cleared temporary constraints. Total temporary constraints: " << num_temporary_constraints << endl;
}

double GurobiSolverInterface::get_infinity() const {
    return GRB_INFINITY;
}

void GurobiSolverInterface::set_objective_coefficients(const vector<double> &coefficients) {
   // assert(coefficients.size() == get_num_variables());
    int num_coefficients = coefficients.size();
    if (!num_coefficients) {
        return;
    }                                                                                // TODO: is there a more elegant way to handle this?
    GRB_CALL(env, GRBsetdblattrarray, model, GRB_DBL_ATTR_OBJ, 0, num_coefficients, const_cast<double *>(coefficients.data()));
    model_dirty = true;
    //cerr << ">>>>> New objective coefficients: [";
    //for (int i = 0; i < num_coefficients; ++i) {
    //    cerr << coefficients[i];
    //    if (i + 1 < num_coefficients) {
    //        cerr << ", ";
    //    }
    //}
    //cerr << "]" << endl;
}

void GurobiSolverInterface::set_objective_coefficient(int index, double coefficient) {
    //assert(index >= 0 && index < get_num_variables());
    GRB_CALL(env, GRBsetdblattrelement, model, GRB_DBL_ATTR_OBJ, index, coefficient);
    model_dirty = true;
    //cerr << ">>>>> New objective coefficient for variable " << index << ": " << coefficient << endl;
}

void GurobiSolverInterface::set_constraint_rhs(int index, double bound) {
    assert(index >= 0 && index < get_num_constraints());
    GRB_CALL(env, GRBsetdblattrelement, model, GRB_DBL_ATTR_RHS, index, bound);
    model_dirty = true;
    //cerr << ">>>>> New right-hand side for constraint " << index << ": " << bound << endl;
}

void GurobiSolverInterface::set_constraint_sense(int index, lp::Sense sense) {
    assert(index >= 0 && index < get_num_constraints());
    if (sense == lp::Sense::GE) {
        GRB_CALL(env, GRBsetcharattrelement, model, GRB_CHAR_ATTR_SENSE, index, GRB_GREATER_EQUAL);
    } else if (sense == lp::Sense::LE) {
        GRB_CALL(env, GRBsetcharattrelement, model, GRB_CHAR_ATTR_SENSE, index, GRB_LESS_EQUAL);
    } else if (sense == lp::Sense::EQ) {
        GRB_CALL(env, GRBsetcharattrelement, model, GRB_CHAR_ATTR_SENSE, index, GRB_EQUAL);
    } else {
        cerr << "Error: Unknown constraint sense." << endl;
        utils::exit_with(utils::ExitCode::SEARCH_CRITICAL_ERROR);
    }
    model_dirty = true;
    //cerr << ">>>>> New sense for constraint " << index << ": " 
    //     << (sense == lp::Sense::GE ? "GE" : (sense == lp::Sense::LE ? "LE" : (sense == lp::Sense::EQ ? "EQ" : "UNKNOWN"))) 
    //     << endl;
}

void GurobiSolverInterface::set_variable_lower_bound(int index, double bound) {
    //assert(index >= 0 && index < get_num_variables());
    GRB_CALL(env, GRBsetdblattrelement, model, GRB_DBL_ATTR_LB, index, bound);
    model_dirty = true;
    //cerr << ">>>>> New lower bound for variable " << index << ": " << bound << endl;
}

void GurobiSolverInterface::set_variable_upper_bound(int index, double bound) {
    //assert(index >= 0 && index < get_num_variables());
    GRB_CALL(env, GRBsetdblattrelement, model, GRB_DBL_ATTR_UB, index, bound);
    model_dirty = true;
    //cerr << ">>>>> New upper bound for variable " << index << ": " << bound << endl;
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
    GRB_CALL(env, GRBwrite, model, filename.c_str());
}

// TODO: implement more detailed failure analysis
void GurobiSolverInterface::print_failure_analysis() const {
    int status = 0;
    GRB_CALL(env, GRBgetintattr, model, GRB_INT_ATTR_STATUS, &status);
}

// TODO: check if a solution is available
bool GurobiSolverInterface::is_infeasible() const {
    int status = 0;
    GRB_CALL(env, GRBgetintattr, model, GRB_INT_ATTR_STATUS, &status);
    //cerr << ">>>>> Is infeasible?  " << (status == GRB_INFEASIBLE) << endl;
    return status == GRB_INFEASIBLE;
}

// TODO: check if a solution is available
bool GurobiSolverInterface::is_unbounded() const {
    int status = 0;
    GRB_CALL(env, GRBgetintattr, model, GRB_INT_ATTR_STATUS, &status);
    //cerr << ">>>>> Is unbounded?  " << (status == GRB_UNBOUNDED) << endl;
    return status == GRB_UNBOUNDED;
}

// TODO: check if a solution is available
bool GurobiSolverInterface::has_optimal_solution() const {
    int status = 0;
    GRB_CALL(env, GRBgetintattr, model, GRB_INT_ATTR_STATUS, &status);
    //cerr << ">>>>> Has optimal solution?  " << (status == GRB_OPTIMAL) << endl;
    return status == GRB_OPTIMAL;
}

// TODO: check if a solution is available
double GurobiSolverInterface::get_objective_value() const {
    double value = 0.0;
    GRB_CALL(env, GRBgetdblattr, model, GRB_DBL_ATTR_OBJVAL, &value);
    //cerr << ">>>>> Objective value: " << value << endl;
    return value;
}

// TODO: check if an optimal solution is available
vector<double> GurobiSolverInterface::extract_solution() const {
    int num_variables = 0;
    GRB_CALL(env, GRBgetintattr, model, GRB_INT_ATTR_NUMVARS, &num_variables);
    vector<double> solution(num_variables);
    if (num_variables > 0) {
        GRB_CALL(env, GRBgetdblattrarray, model, GRB_DBL_ATTR_X, 0, num_variables, solution.data());
    }
    //cerr << ">>>>> Extracted solution: [";
    //for (int i = 0; i < num_variables; ++i) {
    //    cerr << solution[i];
    //    if (i + 1 < num_variables) {
    //        cerr << ", ";
    //    }
    //}
    //cerr << "]" << endl;
    return solution;
}

int GurobiSolverInterface::get_num_variables() const {
    int num_variables = 0;
    GRB_CALL(env, GRBgetintattr, model, GRB_INT_ATTR_NUMVARS, &num_variables);
    //cerr << ">>>>> Number of variables: " << num_variables << endl;
    return num_variables;
}

int GurobiSolverInterface::get_num_constraints() const {
    //cerr << ">>>>> Number of constraints: " << (num_permanent_constraints + num_temporary_constraints) << endl;
    return num_permanent_constraints + num_temporary_constraints;
}

bool GurobiSolverInterface::has_temporary_constraints() const {
    //cerr << ">>>>> Has temporary constraints?  " << (num_temporary_constraints > 0) << endl;
    return num_temporary_constraints > 0;
}

void GurobiSolverInterface::print_statistics() const {
    double runtime = 0.0;
    GRB_CALL(env, GRBgetdblattr, model, GRB_DBL_ATTR_RUNTIME, &runtime);
    cout << "Gurobi runtime: " << runtime << "s" << endl;
}
}
