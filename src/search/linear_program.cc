#include "linear_program.h"

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

LPConstraint::LPConstraint()
    : lower_bound(-numeric_limits<double>::infinity()),
      upper_bound(numeric_limits<double>::infinity()) {
}

LPConstraint::~LPConstraint() {
}

bool LPConstraint::empty() const {
    return variables.empty();
}

void LPConstraint::insert(int index, double coefficient) {
    assert(find(variables.begin(), variables.end(), index) == variables.end());
    variables.push_back(index);
    coefficients.push_back(coefficient);
}

LPVariable::LPVariable(double lower_bound_, double upper_bound_,
                       double objective_coefficient_)
    : lower_bound(lower_bound_),
      upper_bound(upper_bound_),
      objective_coefficient(objective_coefficient_) {
}

LPVariable::~LPVariable() {
}

#ifdef USE_LP

LP::LP(LPSolverType solver_type,
   const std::vector<LPVariable> &variables,
   const std::vector<LPConstraint> &constraints,
   LPObjectiveSense sense)
    : is_initialized(false),
      is_solved(false),
      has_temporary_constraints(false) {
    lp_solver = create_lp_solver(solver_type);
    int num_columns = variables.size();
    int num_rows = constraints.size();
    num_permanent_constraints = num_rows;

    try {
        if (sense == LPObjectiveSense::MINIMIZE) {
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
        // TODO: delete rows or are they owned by matrix?
        matrix->appendRows(num_rows, get_rows(constraints));
        double *col_lb = build_array<LPVariable>(
            variables,
            [](const LPVariable &var) {return var.lower_bound; });
        double *col_ub = build_array<LPVariable>(
            variables,
            [](const LPVariable &var) {return var.upper_bound; });
        double *objective = build_array<LPVariable>(
            variables,
            [](const LPVariable &var) {return var.objective_coefficient; });
        double *row_lb = build_array<LPConstraint>(
            constraints,
            [](const LPConstraint &constraint) {return constraint.lower_bound; });
        double *row_ub = build_array<LPConstraint>(
            constraints,
            [](const LPConstraint &constraint) {return constraint.upper_bound; });
        lp_solver->assignProblem(matrix, col_lb, col_ub, objective, row_lb, row_ub);
    } catch (CoinError &error) {
        handle_coin_error(error);
    }
}

LP::~LP() {
    delete lp_solver;
}

double LP::translate_infinity(double value) const {
    if (value == numeric_limits<double>::infinity())
        return lp_solver->getInfinity();
    else if (value == -numeric_limits<double>::infinity())
        return -lp_solver->getInfinity();
    return value;
}

template<typename T>
double *LP::build_array(const vector<T> &vec,
                        function<double(const T&)> func) const {
    double *result = new double[vec.size()];
    for (size_t i = 0; i < vec.size(); ++i) {
        result[i] = translate_infinity(func(vec[i]));
    }
    return result;
}

CoinPackedVectorBase **LP::get_rows(const std::vector<LPConstraint> &constraints) {
    CoinPackedVectorBase **rows = new CoinPackedVectorBase *[constraints.size()];
    for (size_t i = 0; i < constraints.size(); ++i) {
        const vector <int> &variables = constraints[i].get_variables();
        const vector <double> &coefficients = constraints[i].get_coefficients();
        assert(variables.size() == coefficients.size());
        CoinPackedVector *coin_vector = new CoinPackedVector(false);
        coin_vector->reserve(variables.size());
        for (size_t j = 0; j < variables.size(); ++j) {
            coin_vector->insert(variables[j], translate_infinity(coefficients[j]));
        }
        rows[i] = coin_vector;
    }
    return rows;
}

int LP::add_temporary_constraints(const std::vector<LPConstraint> &constraints) {
    int index_of_first_constraint = get_num_constraints();
    if (!constraints.empty()) {
        try {
            double *row_lb = build_array<LPConstraint>(
                constraints,
                [](const LPConstraint &constraint) {return constraint.lower_bound; });
            double *row_ub = build_array<LPConstraint>(
                constraints,
                [](const LPConstraint &constraint) {return constraint.upper_bound; });
            lp_solver->addRows(constraints.size(),
                               get_rows(constraints), row_lb, row_ub);
        } catch (CoinError &error) {
            handle_coin_error(error);
        }
        has_temporary_constraints = true;
        is_solved = false;
    }
    return index_of_first_constraint;
}

void LP::clear_temporary_constraints() {
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

bool LP::has_optimal_solution() const {
    assert(is_solved);
    try {
        return !lp_solver->isProvenPrimalInfeasible() && lp_solver->isProvenOptimal();
    } catch (CoinError &error) {
        handle_coin_error(error);
    }
}

double LP::get_objective_value() const {
    assert(is_solved);
    try {
        return lp_solver->getObjValue();
    } catch (CoinError &error) {
        handle_coin_error(error);
    }
}

std::vector<double> LP::extract_solution() const {
    assert(is_solved);
    try {
        const double *sol = lp_solver->getColSolution();
        return std::vector<double>(sol, sol + get_num_variables());
    } catch (CoinError &error) {
        handle_coin_error(error);
    }
}

int LP::get_num_variables() const {
    try {
        return lp_solver->getNumCols();
    } catch (CoinError &error) {
        handle_coin_error(error);
    }
}

int LP::get_num_constraints() const {
    try {
        return lp_solver->getNumRows();
    } catch (CoinError &error) {
        handle_coin_error(error);
    }
}

void LP::print_statistics() const {
    cout << "LP variables: " << get_num_variables() << endl;
    cout << "LP constraints: " << get_num_constraints() << endl;
}
#else
LP::LP(LPSolverType solver_type,
   const std::vector<LPVariable> &variables,
   const std::vector<LPConstraint> &constraints,
   LPObjectiveSense sense) {
    // silence unused variable warnings
    unused_parameter(solver_type);
    unused_parameter(variables);
    unused_parameter(constraints);
    unused_parameter(sense);
    cerr << "You must build the planner with the USE_LP symbol defined" << endl;
    exit_with(EXIT_CRITICAL_ERROR);
}

LP::~LP() {
}
#endif
