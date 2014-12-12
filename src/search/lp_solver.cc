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

#ifdef USE_LP

LpConstraint::LpConstraint(double lower_bound_, double upper_bound_)
    : lower_bound(lower_bound_),
      upper_bound(upper_bound_) {
}

LpConstraint::~LpConstraint() {
}

CoinPackedVectorBase *LpConstraint::create_coin_vector() const {
    assert(variables.size() == coefficients.size());
    return new CoinShallowPackedVector(variables.size(),
                                       variables.data(),
                                       coefficients.data(),
                                       false);
}

bool LpConstraint::empty() const {
    return variables.empty();
}

void LpConstraint::insert(int index, double coefficient) {
    assert(find(variables.begin(), variables.end(), index) == variables.end());
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

template<typename T>
double *LpSolver::build_array(const vector<T> &vec,
                        function<double(const T&)> func) const {
    double *result = new double[vec.size()];
    for (size_t i = 0; i < vec.size(); ++i) {
        result[i] = func(vec[i]);
    }
    return result;
}

CoinPackedVectorBase **LpSolver::create_rows(const std::vector<LpConstraint> &constraints) {
    CoinPackedVectorBase **rows = new CoinPackedVectorBase *[constraints.size()];
    for (size_t i = 0; i < constraints.size(); ++i) {
        rows[i] = constraints[i].create_coin_vector();
    }
    return rows;
}

void LpSolver::assign_problem(LPObjectiveSense sense,
                        const std::vector<LpVariable> &variables,
                        const std::vector<LpConstraint> &constraints) {
    int num_columns = variables.size();
    int num_rows = constraints.size();
    num_permanent_constraints = num_rows;
    try {
        /*
          Note that using assignProblem instead of loadProblem, the ownership of
          the data is transfered to the LP solver. It will delete the matrix,
          bounds and the objective so we do not delete them afterwards.
        */
        CoinPackedMatrix *matrix = new CoinPackedMatrix(false, 0, 0);
        matrix->setDimensions(0, num_columns);
        CoinPackedVectorBase **rows = create_rows(constraints);
        matrix->appendRows(num_rows, rows);
        for (size_t i = 0; i < constraints.size(); ++i) {
            delete rows[i];
        }
        delete[] rows;
        double *col_lb = build_array<LpVariable>(
            variables,
            [](const LpVariable &var) {return var.lower_bound; });
        double *col_ub = build_array<LpVariable>(
            variables,
            [](const LpVariable &var) {return var.upper_bound; });
        double *objective = build_array<LpVariable>(
            variables,
            [](const LpVariable &var) {return var.objective_coefficient; });
        double *row_lb = build_array<LpConstraint>(
            constraints,
            [](const LpConstraint &constraint) {return constraint.lower_bound; });
        double *row_ub = build_array<LpConstraint>(
            constraints,
            [](const LpConstraint &constraint) {return constraint.upper_bound; });
        if (sense == LPObjectiveSense::MINIMIZE) {
            lp_solver->setObjSense(1);
        } else {
            lp_solver->setObjSense(-1);
        }
        lp_solver->assignProblem(matrix, col_lb, col_ub, objective, row_lb, row_ub);
    } catch (CoinError &error) {
        handle_coin_error(error);
    }
}

int LpSolver::add_temporary_constraints(const std::vector<LpConstraint> &constraints) {
    int index_of_first_constraint = get_num_constraints();
    if (!constraints.empty()) {
        try {
            double *row_lb = build_array<LpConstraint>(
                constraints,
                [](const LpConstraint &constraint) {return constraint.lower_bound; });
            double *row_ub = build_array<LpConstraint>(
                constraints,
                [](const LpConstraint &constraint) {return constraint.upper_bound; });
            CoinPackedVectorBase **rows = create_rows(constraints);
            lp_solver->addRows(constraints.size(), rows, row_lb, row_ub);
            for (size_t i = 0; i < constraints.size(); ++i) {
                delete rows[i];
            }
            delete[] rows;
        } catch (CoinError &error) {
            handle_coin_error(error);
        }
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
