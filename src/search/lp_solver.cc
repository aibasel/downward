#include "lp_solver.h"

#include "lp_internals.h"
#include "option_parser.h"
#include "utilities.h"

#ifdef USE_LP
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
#include <OsiSolverInterface.hpp>
#include <CoinPackedMatrix.hpp>
#include <CoinPackedVector.hpp>
#pragma GCC diagnostic pop
#endif

#include <cassert>
#include <limits>

using namespace std;

void add_lp_solver_option_to_parser(OptionParser &parser) {
    parser.document_note(
        "Note",
        "to use an LP solver, you must build the planner with USE_LP=1. "
        "See LPBuildInstructions.");
    vector<string> lp_solvers;
    vector<string> lp_solvers_doc;
    lp_solvers.push_back("CLP");
    lp_solvers_doc.push_back("default LP solver shipped with the COIN library");
    lp_solvers.push_back("CPLEX");
    lp_solvers_doc.push_back("commercial solver by IBM");
    lp_solvers.push_back("GUROBI");
    lp_solvers_doc.push_back("commercial solver");
    parser.add_enum_option("lpsolver",
                           lp_solvers,
                           "external solver that should be used to solve linear programs",
                           "CPLEX",
                           lp_solvers_doc);
}

LpConstraint::LpConstraint(double lower_bound_, double upper_bound_)
    : lower_bound(lower_bound_),
      upper_bound(upper_bound_) {
}

LpConstraint::~LpConstraint() {
}

void LpConstraint::clear() {
    variables.clear();
    coefficients.clear();
}

bool LpConstraint::empty() const {
    return variables.empty();
}

void LpConstraint::insert(int index, double coefficient) {
    assert(variables.empty() || variables.back() < index);
    variables.push_back(index);
    coefficients.push_back(coefficient);
}

LpVariable::LpVariable(double lower_bound_, double upper_bound_,
                       double objective_coefficient_)
    : lower_bound(lower_bound_),
      upper_bound(upper_bound_),
      objective_coefficient(objective_coefficient_) {
}

LpVariable::~LpVariable() {
}

#ifdef USE_LP

LpSolver::LpSolver(LpSolverType solver_type)
    : is_initialized(false),
      is_solved(false),
      num_permanent_constraints(0),
      has_temporary_constraints(false) {
    lp_solver = create_lp_solver(solver_type);
}

LpSolver::~LpSolver() {
    delete lp_solver;
}

void LpSolver::clear_temporary_data() {
    elements.clear();
    indices.clear();
    starts.clear();
    col_lb.clear();
    col_ub.clear();
    objective.clear();
    row_lb.clear();
    row_ub.clear();
    rows.clear();
}

void LpSolver::load_problem(LPObjectiveSense sense,
                            const std::vector<LpVariable> &variables,
                            const std::vector<LpConstraint> &constraints) {
    int num_columns = variables.size();
    int num_rows = constraints.size();
    num_permanent_constraints = num_rows;
    is_initialized = false;
    clear_temporary_data();
    for (const LpVariable &var : variables) {
        col_lb.push_back(var.lower_bound);
        col_ub.push_back(var.upper_bound);
        objective.push_back(var.objective_coefficient);
    }
    for (const LpConstraint &constraint : constraints) {
        row_lb.push_back(constraint.lower_bound);
        row_ub.push_back(constraint.upper_bound);
    }

    if (sense == LPObjectiveSense::MINIMIZE) {
        lp_solver->setObjSense(1);
    } else {
        lp_solver->setObjSense(-1);
    }

    for (size_t i = 0; i < constraints.size(); ++i) {
        const LpConstraint &constraint = constraints[i];
        const vector<int> &vars = constraint.get_variables();
        const vector<double> &coeffs = constraint.get_coefficients();
        assert(vars.size() == coeffs.size());
        starts.push_back(elements.size());
        indices.insert(indices.end(), vars.begin(), vars.end());
        elements.insert(elements.end(), coeffs.begin(), coeffs.end());
    }
    /*
      There are two ways to pass the lengths of vectors to a CoinMatrix:
      1) 'starts' contains one enty per vector and we pass a separate array
         of vector 'lengths' to the ctor
      2) If there are no gaps in the elements, we can also add elements.size()
         as a last entry in the vector 'starts' and leave the parameter for
         'lengths' at its default (0).
      OSI recreates the 'lengths' array in any case and uses optimized code
      for the second case, so we use it here.
     */
    starts.push_back(elements.size());
    try {
        CoinPackedMatrix matrix(false,
                                num_columns,
                                num_rows,
                                elements.size(),
                                elements.data(),
                                indices.data(),
                                starts.data(),
                                0);
        lp_solver->loadProblem(matrix,
                               col_lb.data(),
                               col_ub.data(),
                               objective.data(),
                               row_lb.data(),
                               row_ub.data());
    } catch (CoinError &error) {
        handle_coin_error(error);
    }
    clear_temporary_data();
}

int LpSolver::add_temporary_constraints(const std::vector<LpConstraint> &constraints) {
    int index_of_first_constraint = get_num_constraints();
    if (!constraints.empty()) {
        clear_temporary_data();
        try {
            int num_rows = constraints.size();
            for (int i = 0; i < num_rows; ++i) {
                const LpConstraint &constraint = constraints[i];
                row_lb.push_back(constraint.lower_bound);
                row_ub.push_back(constraint.upper_bound);
                rows.push_back(new CoinShallowPackedVector(
                    constraint.get_variables().size(),
                    constraint.get_variables().data(),
                    constraint.get_coefficients().data(),
                    false));
            }

            lp_solver->addRows(num_rows,
                               rows.data(), row_lb.data(), row_ub.data());
            for (auto row : rows) {
                delete row;
            }
        } catch (CoinError &error) {
            handle_coin_error(error);
        }
        clear_temporary_data();
        has_temporary_constraints = true;
        is_solved = false;
    }
    return index_of_first_constraint;
}

void LpSolver::clear_temporary_constraints() {
    if (has_temporary_constraints) {
        try {
            lp_solver->restoreBaseModel(num_permanent_constraints);
        } catch (CoinError &error) {
            handle_coin_error(error);
        }
        has_temporary_constraints = false;
    }
}

double LpSolver::get_infinity() {
    try {
        return lp_solver->getInfinity();
    } catch (CoinError &error) {
        handle_coin_error(error);
    }
}

void LpSolver::set_objective_coefficient(int index, double coefficient) {
    assert(index < get_num_variables());
    try {
        lp_solver->setObjCoeff(index, coefficient);
    } catch (CoinError &error) {
        handle_coin_error(error);
    }
    is_solved = false;
}

void LpSolver::set_constraint_lower_bound(int index, double bound) {
    assert(index < get_num_constraints());
    try {
        lp_solver->setRowLower(index, bound);
    } catch (CoinError &error) {
        handle_coin_error(error);
    }
    is_solved = false;
}

void LpSolver::set_constraint_upper_bound(int index, double bound) {
    assert(index < get_num_constraints());
    try {
        lp_solver->setRowUpper(index, bound);
    } catch (CoinError &error) {
        handle_coin_error(error);
    }
    is_solved = false;
}

void LpSolver::set_variable_lower_bound(int index, double bound) {
    assert(index < get_num_variables());
    try {
        lp_solver->setColLower(index, bound);
    } catch (CoinError &error) {
        handle_coin_error(error);
    }
    is_solved = false;
}

void LpSolver::set_variable_upper_bound(int index, double bound) {
    assert(index < get_num_variables());
    try {
        lp_solver->setColUpper(index, bound);
    } catch (CoinError &error) {
        handle_coin_error(error);
    }
    is_solved = false;
}

void LpSolver::solve() {
    try {
        if (is_initialized) {
            lp_solver->resolve();
        } else {
            lp_solver->initialSolve();
            is_initialized = true;
        }
        if (lp_solver->isAbandoned()) {
            // The documentation of OSI is not very clear here but memory seems
            // to be the most common cause for this in our case.
            cerr << "Abandoned LP during resolve. "
                 << "Reasons include \"numerical difficulties\" and running out of memory." << endl;
            exit_with(EXIT_CRITICAL_ERROR);
        }
        is_solved = true;
    } catch (CoinError &error) {
        handle_coin_error(error);
    }
}

bool LpSolver::has_optimal_solution() const {
    assert(is_solved);
    try {
        return !lp_solver->isProvenPrimalInfeasible() && lp_solver->isProvenOptimal();
    } catch (CoinError &error) {
        handle_coin_error(error);
    }
}

double LpSolver::get_objective_value() const {
    assert(is_solved);
    try {
        return lp_solver->getObjValue();
    } catch (CoinError &error) {
        handle_coin_error(error);
    }
}

std::vector<double> LpSolver::extract_solution() const {
    assert(is_solved);
    try {
        const double *sol = lp_solver->getColSolution();
        return std::vector<double>(sol, sol + get_num_variables());
    } catch (CoinError &error) {
        handle_coin_error(error);
    }
}

int LpSolver::get_num_variables() const {
    try {
        return lp_solver->getNumCols();
    } catch (CoinError &error) {
        handle_coin_error(error);
    }
}

int LpSolver::get_num_constraints() const {
    try {
        return lp_solver->getNumRows();
    } catch (CoinError &error) {
        handle_coin_error(error);
    }
}

void LpSolver::print_statistics() const {
    cout << "LP variables: " << get_num_variables() << endl;
    cout << "LP constraints: " << get_num_constraints() << endl;
}
#endif
