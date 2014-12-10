#include "linear_program.h"

#include "utilities.h"

#include <CoinPackedMatrix.hpp>
#include <CoinPackedVector.hpp>

#include <cassert>
#include <limits>

using namespace std;

LPConstraint::LPConstraint()
    : lower_bound(-numeric_limits<double>::infinity()),
      upper_bound(numeric_limits<double>::infinity()) {
}

LPConstraint::~LPConstraint() {
}

bool LPConstraint::empty() const {
    return indices.empty();
}

void LPConstraint::insert(int index, double coefficient) {
    assert(find(indices.begin(), indices.end(), index) == indices.end());
    indices.push_back(index);
    coefficients.push_back(coefficient);
}

LP::LP(LPSolverType solver_type,
   const std::vector<LPVariable> &variables,
   const std::vector<LPConstraint> &constraints,
   LPObjectiveSense sense)
    : has_temporary_constraints(false) {
    lp_solver = create_lp_solver(solver_type);
    //----------------------------------------------------------------------
    int num_columns = variables.size();
    int num_rows = constraints.size();
    num_permanent_constraints = num_rows;

    try {
        if (sense == MINIMIZE) {
            lp_solver->setObjSense(1);
        } else {
            lp_solver->setObjSense(-1);
        }
        /*
          Note that using assignProblem instead of loadProblem, the ownership of
          the data is transfered to the LP solver. It will delete the matrix,
          bounds and the objective so we do not delete them afterwards.
        */
        CoinPackedMatrix *matrix = new CoinPackedMatrix(false, 0, 0);
        matrix->setDimensions(0, num_columns);
        matrix->appendRows(num_rows, get_rows(constraints));
        double *col_lb = get_lower_bounds<LPVariable>(variables);
        double *col_ub = get_upper_bounds<LPVariable>(variables);
        double *objective = get_objectives(variables);
        double *row_lb = get_lower_bounds<LPConstraint>(constraints);
        double *row_ub = get_upper_bounds<LPConstraint>(constraints);
        lp_solver->assignProblem(matrix, col_lb, col_ub, objective, row_lb, row_ub);
        lp_solver->initialSolve();
        is_solved = true;
    } catch (CoinError &error) {
        handle_coin_error(error);
    }
}

LP::~LP() {
    delete lp_solver;
}

double LP::translate_infinity(double value){
    if (value == numeric_limits<double>::infinity())
        return lp_solver->getInfinity();
    else if (value == -numeric_limits<double>::infinity())
        return -lp_solver->getInfinity();
    return value;
}

template<typename T>
double *LP::get_lower_bounds(const std::vector<T> &collection){
    double *result = new double[collection.size()];
    for (size_t i = 0; i < collection.size(); ++i) {
        result[i] = translate_infinity(collection[i].lower_bound);
    }
    return result;
}

template<typename T>
double *LP::get_upper_bounds(const std::vector<T> &collection) {
    double *result = new double[collection.size()];
    for (size_t i = 0; i < collection.size(); ++i) {
        result[i] = translate_infinity(collection[i].upper_bound);
    }
    return result;
}

double *LP::get_objectives(const std::vector<LPVariable> &variables) {
    double *result = new double[variables.size()];
    for (size_t i = 0; i < variables.size(); ++i) {
        result[i] = translate_infinity(variables[i].objective);
    }
    return result;
}

CoinPackedVectorBase **LP::get_rows(const std::vector<LPConstraint> &constraints) {
    CoinPackedVectorBase **rows = new CoinPackedVectorBase *[constraints.size()];
    for (size_t i = 0; i < constraints.size(); ++i) {
        const vector <int> &indices = constraints[i].get_indices();
        const vector <double> &coefficients = constraints[i].get_coefficients();
        assert(indices.size() == coefficients.size());
        CoinPackedVector *coin_vector = new CoinPackedVector(false);
        coin_vector->reserve(indices.size());
        for (size_t j = 0; j < indices.size(); ++j) {
            coin_vector->insert(indices[j], translate_infinity(coefficients[j]));
        }
        rows[i] = coin_vector;
    }
    return rows;
}

int LP::add_temporary_constraints(const std::vector<LPConstraint> &constraints) {
    int index_of_first_constraint = get_num_constraints();
    if (!constraints.empty()) {
        try {
            lp_solver->addRows(constraints.size(),
                               get_rows(constraints),
                               get_lower_bounds(constraints),
                               get_upper_bounds(constraints));
        } catch (CoinError &error) {
            handle_coin_error(error);
        }
        has_temporary_constraints = true;
        is_solved = false;
    }
    return index_of_first_constraint;
}

void LP::remove_temporary_constraints() {
    if (has_temporary_constraints) {
        try {
            lp_solver->restoreBaseModel(num_permanent_constraints);
        } catch (CoinError &error) {
            handle_coin_error(error);
        }
        has_temporary_constraints = false;
    }
}

void LP::set_objective_coefficient(int index, double coefficient) {
    assert(index < get_num_variables());
    try {
        lp_solver->setObjCoeff(index, translate_infinity(coefficient));
    } catch (CoinError &error) {
        handle_coin_error(error);
    }
    is_solved = false;
}

void LP::set_constraint_lower_bound(int index, double bound) {
    assert(index < get_num_constraints());
    try {
        lp_solver->setRowLower(index, translate_infinity(bound));
    } catch (CoinError &error) {
        handle_coin_error(error);
    }
    is_solved = false;
}

void LP::set_constraint_upper_bound(int index, double bound) {
    assert(index < get_num_constraints());
    try {
        lp_solver->setRowUpper(index, translate_infinity(bound));
    } catch (CoinError &error) {
        handle_coin_error(error);
    }
    is_solved = false;
}

void LP::set_variable_lower_bound(int index, double bound) {
    assert(index < get_num_variables());
    try {
        lp_solver->setColLower(index, translate_infinity(bound));
    } catch (CoinError &error) {
        handle_coin_error(error);
    }
    is_solved = false;
}

void LP::set_variable_upper_bound(int index, double bound) {
    assert(index < get_num_variables());
    try {
        lp_solver->setColUpper(index, translate_infinity(bound));
    } catch (CoinError &error) {
        handle_coin_error(error);
    }
    is_solved = false;
}

void LP::solve() {
    try {
        lp_solver->resolve();
        if (lp_solver->isAbandoned()) {
            // The documentation of OSI is not very clear here but memory seems
            // to be the most common cause for this in our case.
            cerr << "Abandoned LP during resolve. "
                 << "Reasons include \"numerical difficuties\" and running out of memory." << endl;
            exit_with(EXIT_CRITICAL_ERROR);
        }
        is_solved = true;
    } catch (CoinError &error) {
        handle_coin_error(error);
    }
}

bool LP::has_optimal_solution() const {
    assert(is_solved);
    try {
        return !lp_solver->isProvenPrimalInfeasible() && lp_solver->isProvenOptimal();
    } catch (CoinError &error) {
        handle_coin_error(error);
    }
}

double LP::objective_value() const {
    assert(is_solved);
    try {
        return lp_solver->getObjValue();
    } catch (CoinError &error) {
        handle_coin_error(error);
    }
}

void LP::extract_solution(std::vector<double> &solution) const {
    assert(is_solved);
    solution.resize(get_num_variables());
    try {
        const double *sol = lp_solver->getColSolution();
        for (size_t i = 0; i < solution.size(); ++i) {
            solution[i] = sol[i];
        }
    } catch (CoinError &error) {
        handle_coin_error(error);
    }
}

int LP::get_num_variables() const {
    return lp_solver->getNumCols();
}

int LP::get_num_constraints() const {
    return lp_solver->getNumRows();
}

void LP::print_statistics() const {
    cout << "LP variables: " << get_num_variables() << endl;
    cout << "LP constraints: " << get_num_constraints() << endl;
}
