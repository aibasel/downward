#include "cplex_solver_interface.h"
#ifdef HAS_CPLEX

#include "lp_solver.h"

#include "../utils/language.h"
#include "../utils/logging.h"

#include <cstring>
#include <numeric>

using namespace std;

namespace lp {
NO_RETURN
static void handle_cplex_error(CPXENVptr env, int error_code) {
    char buffer[CPXMESSAGEBUFSIZE];
    const char *error_string = CPXgeterrorstring(env, error_code, buffer);
    if (error_string) {
        cerr << "CPLEX error: " << error_string << endl;
    } else {
        cerr << "CPLEX error: Unknown error code " << error_code << endl;
    }
    utils::exit_with(utils::ExitCode::SEARCH_CRITICAL_ERROR);
}

/* Make a call to a CPLEX API function checking its return status. */
template<typename Func, typename... Args>
static void CPX_CALL(Func cpxfunc, CPXENVptr env, Args&&... args) {
    int status = cpxfunc(env, forward<Args>(args)...);
    if (status) {
        handle_cplex_error(env, status);
    }
}

CPXLPptr createProblem(CPXENVptr env, const string &name) {
    int status = 0;
    CPXLPptr problem = CPXcreateprob(env, &status, name.c_str());
    if (status) {
        handle_cplex_error(env, status);
    }
    return problem;
}

void freeProblem(CPXENVptr env, CPXLPptr *problem) {
    CPX_CALL(CPXfreeprob, env, problem);
}

static tuple<char, double, double> bounds_to_sense_rhs_range(double lb, double ub) {
    if (lb <= -CPX_INFBOUND && ub >= CPX_INFBOUND) {
        // CPLEX does not support <= or >= constraints without bounds.
        return {'R', -CPX_INFBOUND, 2*CPX_INFBOUND};
    } else if (lb <= -CPX_INFBOUND) {
        return {'L', ub, 0};
    } else if (ub >= CPX_INFBOUND) {
        return {'G', lb, 0};
    } else if (lb == ub) {
        return {'E', lb, 0};
    } else {
        return {'R', lb, ub - lb};
    }
}

CplexSolverInterface::CplexSolverInterface()
    : env(nullptr), problem(nullptr), is_mip(false),
      num_permanent_constraints(0), num_unsatisfiable_constraints(0),
      num_unsatisfiable_temp_constraints(0) {
    int status = 0;
    env = CPXopenCPLEX(&status);
    if (status) {
        cerr << "Could not construct CPLEX interface (error_code: " << status
             << ")." << endl;
        utils::exit_with(utils::ExitCode::SEARCH_CRITICAL_ERROR);
    }
    CPX_CALL(CPXsetintparam, env, CPX_PARAM_THREADS, 1);
    // TODO handle output, catch oom
}

CplexSolverInterface::~CplexSolverInterface() {
    int status = CPXcloseCPLEX(&env);
    if (status) {
        cerr << "Failed to close CPLEX interface (error_code: " << status
             << ")." << endl;
        utils::exit_with(utils::ExitCode::SEARCH_CRITICAL_ERROR);
    }
}

void CplexSolverInterface::clear_temporary_data() {
    elements.clear();
    indices.clear();
    starts.clear();
    col_lb.clear();
    col_ub.clear();
    col_type.clear();
    objective.clear();
    row_rhs.clear();
    row_sense.clear();
    row_range_values.clear();
    row_range_indices.clear();
}

void CplexSolverInterface::add_variables(const named_vector::NamedVector<LPVariable> &variables) {
    clear_temporary_data();

    int num_variables = variables.size();
    col_lb.reserve(num_variables);
    col_ub.reserve(num_variables);
    col_type.reserve(num_variables);
    objective.reserve(num_variables);
    for (const LPVariable &var : variables) {
        col_lb.push_back(var.lower_bound);
        col_ub.push_back(var.upper_bound);
        if (var.is_integer) {
            col_type.push_back(CPX_INTEGER);
        } else {
            col_type.push_back(CPX_CONTINUOUS);
        }
        objective.push_back(var.objective_coefficient);
    }

    // Optionally add variable names.
    char **col_names = nullptr;
    if (variables.has_names()) {
        col_names = new char *[variables.size()];
        for (int i = 0; i < variables.size(); ++i) {
            col_names[i] = variables.get_name(i).data();
        }
    }

    CPX_CALL(CPXnewcols, env, problem, num_variables, objective.data(),
             col_lb.data(), col_ub.data(), col_type.data(), col_names);

    if (col_names) {
        delete[] col_names;
    }

    clear_temporary_data();
}

void CplexSolverInterface::add_constraints(const named_vector::NamedVector<LPConstraint> &constraints, bool temporary) {
    clear_temporary_data();
    int constraint_id = get_num_constraints();
    for (const LPConstraint &constraint : constraints) {
        double lb = constraint.get_lower_bound();
        double ub = constraint.get_upper_bound();
        const auto &[sense, rhs, range] = bounds_to_sense_rhs_range(lb, ub);
        if (lb > ub) {
            if (temporary) {
                ++num_unsatisfiable_temp_constraints;
            } else {
                ++num_unsatisfiable_constraints;
            }
        }
        row_sense.push_back(sense);
        row_rhs.push_back(rhs);
        if (sense == 'R') {
            row_range_indices.push_back(constraint_id);
            row_range_values.push_back(range);
        }
        ++constraint_id;
    }

    // Optionally add constraint names.
    char **row_names = nullptr;
    if (constraints.has_names()) {
        row_names = new char *[constraints.size()];
        for (int i = 0; i < constraints.size(); ++i) {
            row_names[i] = constraints.get_name(i).data();
        }
    }

    // Collect non-zero matrix entries.
    for (const LPConstraint &constraint : constraints) {
        const vector<int> &vars = constraint.get_variables();
        const vector<double> &coeffs = constraint.get_coefficients();
        assert(vars.size() == coeffs.size());
        starts.push_back(elements.size());
        indices.insert(indices.end(), vars.begin(), vars.end());
        elements.insert(elements.end(), coeffs.begin(), coeffs.end());
    }
    starts.push_back(elements.size());

    // CPXaddrows can add new variables as well, but we do not want any.
    static const int num_extra_columns = 0;
    char **extra_column_names = nullptr;
    CPX_CALL(CPXaddrows, env, problem, num_extra_columns, row_sense.size(),
             indices.size(), row_rhs.data(), row_sense.data(),
             starts.data(), indices.data(), elements.data(),
             extra_column_names, row_names);

    if (row_names) {
        delete[] row_names;
    }

    /*
      If there are any ranged rows, we have to set up their ranges with a
      separate call.
    */
    if (!row_range_indices.empty()) {
        CPX_CALL(CPXchgrngval, env, problem, row_range_indices.size(),
                 row_range_indices.data(), row_range_values.data());
    }

    clear_temporary_data();
}

pair<double, double> CplexSolverInterface::get_constraint_bounds(int index) {
    char sense;
    CPX_CALL(CPXgetsense, env, problem, &sense, index, index);
    double rhs;
    CPX_CALL(CPXgetrhs, env, problem, &rhs, index, index);
    switch (sense) {
    case 'L':
        return {-CPX_INFBOUND, rhs};
    case 'G':
        return {rhs, CPX_INFBOUND};
    case 'E':
        return {rhs, rhs};
    case 'R':
        double range_value;
        CPX_CALL(CPXgetrhs, env, problem, &range_value, index, index);
        if (range_value > 0) {
            return {rhs, rhs + range_value};
        } else {
            return {rhs + range_value, rhs};
        }
    default:
        ABORT("CPXgetsense returned unknown value.");
    }
}

bool CplexSolverInterface::is_trivially_unsolvable() const {
    return num_unsatisfiable_constraints + num_unsatisfiable_temp_constraints > 0;
}

void CplexSolverInterface::change_constraint_bounds(int index,
    double current_lb, double current_ub, double lb, double ub) {
    const auto &[sense, rhs, range] = bounds_to_sense_rhs_range(lb, ub);

    CPX_CALL(CPXchgsense, env, problem, 1, &index, &sense);
    CPX_CALL(CPXchgrhs, env, problem, 1, &index, &rhs);
    CPX_CALL(CPXchgrngval, env, problem, 1, &index, &range);

    if (current_lb > current_ub && lb <= ub) {
        if (index < num_permanent_constraints) {
            --num_unsatisfiable_constraints;
        } else {
            --num_unsatisfiable_temp_constraints;
        }
    } else if (current_lb <= current_ub && lb > ub) {
        if (index < num_permanent_constraints) {
            ++num_unsatisfiable_constraints;
        } else {
            ++num_unsatisfiable_temp_constraints;
        }
    }
}

void CplexSolverInterface::load_problem(const LinearProgram &lp) {
    cout << "loading problem" << endl;
    if (problem) {
        freeProblem(env, &problem);
    }
    const named_vector::NamedVector<LPVariable> &variables = lp.get_variables();
    const named_vector::NamedVector<LPConstraint> &constraints = lp.get_constraints();

    problem = createProblem(env, "");
    add_variables(variables);
    add_constraints(constraints, false);
    num_permanent_constraints = constraints.size();
    is_mip = any_of(variables.begin(), variables.end(), [](const LPVariable &var) {
        return var.is_integer;
    });
    if (is_mip) {
        CPX_CALL(CPXchgprobtype, env, problem, CPXPROB_MILP);
    } else {
        CPX_CALL(CPXchgprobtype, env, problem, CPXPROB_LP);
    }

    // Set objective sense.
    if (lp.get_sense() == LPObjectiveSense::MINIMIZE) {
        CPX_CALL(CPXchgobjsen, env, problem, CPX_MIN);
    } else {
        CPX_CALL(CPXchgobjsen, env, problem, CPX_MAX);
    }

    /*
      Set objective name (there seems to be no direct way to do this, so
      we use it as the problem name).
    */
    if (!lp.get_objective_name().empty()) {
        CPX_CALL(CPXchgprobname, env, problem, lp.get_objective_name().c_str());
    }
}

void CplexSolverInterface::add_temporary_constraints(
    const named_vector::NamedVector<LPConstraint> &constraints) {
    add_constraints(constraints, true);
}

void CplexSolverInterface::clear_temporary_constraints() {
    int start = num_permanent_constraints;
    int end = get_num_constraints() - 1;
    if (start < end) {
        CPX_CALL(CPXdelrows, env, problem, start, end);
        num_unsatisfiable_temp_constraints = 0;
    }
}

double CplexSolverInterface::get_infinity() const {
    return CPX_INFBOUND;
}

void CplexSolverInterface::set_objective_coefficients(const std::vector<double> &coefficients) {
    clear_temporary_constraints();
    indices.resize(coefficients.size());
    iota(indices.begin(), indices.end(), 0);
    CPX_CALL(CPXchgobj, env, problem, coefficients.size(), indices.data(), coefficients.data());
    clear_temporary_constraints();
}

void CplexSolverInterface::set_objective_coefficient(int index, double coefficient) {
    CPX_CALL(CPXchgobj, env, problem, 1, &index, &coefficient);
}

void CplexSolverInterface::set_constraint_lower_bound(int index, double bound) {
    const auto &[current_lb, current_ub] = get_constraint_bounds(index);
    change_constraint_bounds(index, current_lb, current_ub, bound, current_ub);
}

void CplexSolverInterface::set_constraint_upper_bound(int index, double bound) {
    const auto &[current_lb, current_ub] = get_constraint_bounds(index);
    change_constraint_bounds(index, current_lb, current_ub, current_lb, bound);
}

void CplexSolverInterface::set_variable_lower_bound(int index, double bound) {
    static const char bound_type = 'L';
    CPX_CALL(CPXchgbds, env, problem, 1, &index, &bound_type, &bound);
}

void CplexSolverInterface::set_variable_upper_bound(int index, double bound) {
    static const char bound_type = 'U';
    CPX_CALL(CPXchgbds, env, problem, 1, &index, &bound_type, &bound);
}

void CplexSolverInterface::set_mip_gap(double gap) {
    CPX_CALL(CPXsetdblparam, env, CPXPARAM_MIP_Tolerances_MIPGap, gap);
}

void CplexSolverInterface::solve() {
    if (is_trivially_unsolvable()) {
        return;
    } else if (is_mip) {
        CPX_CALL(CPXmipopt, env, problem);
    } else {
        CPX_CALL(CPXdualopt, env, problem);
    }
}

void CplexSolverInterface::write_lp(const string &filename) const {
    if (is_trivially_unsolvable()) {
        cerr << "The LP has trivially unsatisfiable constraints that are not "
             << "accurately represented in CPLEX. Writing it to a file would "
             << "misrepresent the LP." << endl;
        utils::exit_with(utils::ExitCode::SEARCH_CRITICAL_ERROR);
    }
    // By not passing in a filetype, we let CPLEX infer it from the filename.
    static const char *filetype = nullptr;
    CPX_CALL(CPXwriteprob, env, problem, filename.c_str(), filetype);
}

void CplexSolverInterface::print_failure_analysis() const {
    if (is_trivially_unsolvable()) {
        cout << "LP/MIP is infeasible because of a trivially unsatisfiable "
                "constraint" << endl;
        return;
    }
    int status = CPXgetstat(env, problem);
    switch (status) {
        case CPX_STAT_OPTIMAL:
        case CPXMIP_OPTIMAL:
            cout << "LP/MIP has an optimal solution." << endl;
            break;
        case CPX_STAT_UNBOUNDED:
            cout << "LP/MIP is unbounded" << endl;
            break;
        case CPX_STAT_INFEASIBLE:
        case CPXMIP_INFEASIBLE:
            cout << "LP/MIP is infeasible" << endl;
            break;
        default:
            cout << "Unexpected status after solving LP/MIP: " << status << endl;
    }
}

bool CplexSolverInterface::is_infeasible() const {
    if (is_trivially_unsolvable()) {
        return true;
    }
    int status = CPXgetstat(env, problem);
    return status == CPX_STAT_INFEASIBLE || status == CPXMIP_INFEASIBLE;
}

bool CplexSolverInterface::is_unbounded() const {
    if (is_trivially_unsolvable()) {
        return false;
    }
    int status = CPXgetstat(env, problem);
    return status == CPX_STAT_UNBOUNDED;
}

bool CplexSolverInterface::has_optimal_solution() const {
    if (is_trivially_unsolvable()) {
        return false;
    }
    int status = CPXgetstat(env, problem);
    switch (status) {
        case CPX_STAT_OPTIMAL:
        case CPXMIP_OPTIMAL:
            return true;
        case CPX_STAT_UNBOUNDED:
        case CPX_STAT_INFEASIBLE:
        case CPXMIP_INFEASIBLE:
            return false;
        default:
            cerr << "Unexpected status after solving LP/MIP: " << status << endl;
            utils::exit_with(utils::ExitCode::SEARCH_CRITICAL_ERROR);
    }
}

double CplexSolverInterface::get_objective_value() const {
    assert(has_optimal_solution());
    double value;
    CPX_CALL(CPXgetobjval, env, problem, &value);
    return value;
}

vector<double> CplexSolverInterface::extract_solution() const {
    assert(has_optimal_solution());
    int num_variables = get_num_variables();
    vector<double> solution(num_variables);
    CPX_CALL(CPXgetx, env, problem, solution.data(), 0, num_variables - 1);
    return solution;
}

int CplexSolverInterface::get_num_variables() const {
    return CPXgetnumcols(env, problem);
}

int CplexSolverInterface::get_num_constraints() const {
    return CPXgetnumrows(env, problem);
}

bool CplexSolverInterface::has_temporary_constraints() const {
    return num_permanent_constraints < get_num_constraints();
}

void CplexSolverInterface::print_statistics() const {
    utils::g_log << "LP variables: " << get_num_variables() << endl;
    utils::g_log << "LP constraints: " << get_num_constraints() << endl;
    utils::g_log << "LP non-zero entries: " << CPXgetnumnz(env, problem) << endl;
}
}
#endif
