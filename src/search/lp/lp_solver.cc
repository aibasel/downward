#include "lp_solver.h"

#include "lp_internals.h"

#include "../plugins/plugin.h"
#include "../utils/collections.h"
#include "../utils/logging.h"
#include "../utils/system.h"

#ifdef USE_LP
#ifdef __GNUG__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
#endif

/*
   OSI uses the keyword 'register' which was deprecated for a while and removed
   in C++ 17. Most compilers ignore it but clang 14 complains if it is still used.
*/
#ifdef __clang__
#pragma clang diagnostic ignored "-Wkeyword-macro"
#endif
#define register

#include <OsiSolverInterface.hpp>
#include <CoinPackedMatrix.hpp>
#include <CoinPackedVector.hpp>
#ifdef __GNUG__
#pragma GCC diagnostic pop
#endif
#endif

#include <cassert>
#include <iostream>
#include <numeric>

using namespace std;
using utils::ExitCode;

namespace lp {
void add_lp_solver_option_to_feature(plugins::Feature &feature) {
    feature.add_option<LPSolverType>(
        "lpsolver",
        "external solver that should be used to solve linear programs",
        "cplex");

    feature.document_note(
        "Note",
        "to use an LP solver, you must build the planner with LP support. "
        "See LPBuildInstructions.");
}

LPConstraint::LPConstraint(double lower_bound, double upper_bound)
    : lower_bound(lower_bound),
      upper_bound(upper_bound) {
}

void LPConstraint::clear() {
    variables.clear();
    coefficients.clear();
}

bool LPConstraint::empty() const {
    return variables.empty();
}

void LPConstraint::insert(int index, double coefficient) {
    variables.push_back(index);
    coefficients.push_back(coefficient);
}

ostream &LPConstraint::dump(ostream &stream, const LinearProgram *program) {
    double infinity = numeric_limits<double>::infinity();
    if (program) {
        infinity = program->get_infinity();
    }
    if (lower_bound != -infinity) {
        stream << lower_bound << " <= ";
    }
    for (size_t i = 0; i < variables.size(); ++i) {
        if (i != 0)
            stream << " + ";
        int variable = variables[i];
        string variable_name;
        if (program && program->get_variables().has_names() && !program->get_variables().get_name(variable).empty()) {
            variable_name = program->get_variables().get_name(variable);
        } else {
            variable_name = "v" + to_string(variable);
        }
        stream << coefficients[i] << " * " << variable_name;
    }
    if (upper_bound != infinity) {
        stream << " <= " << upper_bound;
    } else if (lower_bound == -infinity) {
        stream << " <= infinity";
    }
    return stream;
}

LPVariable::LPVariable(double lower_bound, double upper_bound,
                       double objective_coefficient, bool is_integer)
    : lower_bound(lower_bound),
      upper_bound(upper_bound),
      objective_coefficient(objective_coefficient),
      is_integer(is_integer) {
}

named_vector::NamedVector<LPVariable> &LinearProgram::get_variables() {
    return variables;
}

named_vector::NamedVector<LPConstraint> &LinearProgram::get_constraints() {
    return constraints;
}

double LinearProgram::get_infinity() const {
    return infinity;
}

LPObjectiveSense LinearProgram::get_sense() const {
    return sense;
}

const named_vector::NamedVector<LPVariable> &LinearProgram::get_variables() const {
    return variables;
}

const named_vector::NamedVector<LPConstraint> &LinearProgram::get_constraints() const {
    return constraints;
}

const string &LinearProgram::get_objective_name() const {
    return objective_name;
}

void LinearProgram::set_objective_name(string name) {
    objective_name = name;
}

LPSolver::~LPSolver() {
}

#ifdef USE_LP

LPSolver::LPSolver(LPSolverType solver_type)
    : is_initialized(false),
      is_mip(false),
      is_solved(false),
      num_permanent_constraints(0),
      has_temporary_constraints_(false) {
    try {
        lp_solver = create_lp_solver(solver_type);
    } catch (CoinError &error) {
        handle_coin_error(error);
    }
}

void LPSolver::clear_temporary_data() {
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

void LPSolver::load_problem(const LinearProgram &lp) {
    clear_temporary_data();
    is_mip = false;
    is_initialized = false;
    num_permanent_constraints = lp.get_constraints().size();

    for (const LPVariable &var : lp.get_variables()) {
        col_lb.push_back(var.lower_bound);
        col_ub.push_back(var.upper_bound);
        objective.push_back(var.objective_coefficient);
    }

    for (const LPConstraint &constraint : lp.get_constraints()) {
        row_lb.push_back(constraint.get_lower_bound());
        row_ub.push_back(constraint.get_upper_bound());
    }

    for (const LPConstraint &constraint : lp.get_constraints()) {
        const vector<int> &vars = constraint.get_variables();
        assert(utils::all_values_unique(vars));
        const vector<double> &coeffs = constraint.get_coefficients();
        assert(vars.size() == coeffs.size());
        starts.push_back(elements.size());
        indices.insert(indices.end(), vars.begin(), vars.end());
        elements.insert(elements.end(), coeffs.begin(), coeffs.end());
    }
    /*
      There are two ways to pass the lengths of vectors to a CoinMatrix:
      1) 'starts' contains one entry per vector and we pass a separate array
         of vector 'lengths' to the constructor.
      2) If there are no gaps in the elements, we can also add elements.size()
         as a last entry in the vector 'starts' and leave the parameter for
         'lengths' at its default (0).
      OSI recreates the 'lengths' array in any case and uses optimized code
      for the second case, so we use it here.
     */
    starts.push_back(elements.size());

    try {
        CoinPackedMatrix matrix(false,
                                lp.get_variables().size(),
                                lp.get_constraints().size(),
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
        for (int i = 0; i < static_cast<int>(lp.get_variables().size()); ++i) {
            if (lp.get_variables()[i].is_integer) {
                lp_solver->setInteger(i);
                is_mip = true;
            }
        }

        /*
          We set the objective sense after loading because the SoPlex
          interfaces of all OSI versions <= 0.108.4 ignore it when it is
          set earlier. See issue752 for details.
        */
        if (lp.get_sense() == LPObjectiveSense::MINIMIZE) {
            lp_solver->setObjSense(1);
        } else {
            lp_solver->setObjSense(-1);
        }

        if (!lp.get_objective_name().empty()) {
            lp_solver->setObjName(lp.get_objective_name());
        } else if (lp.get_variables().has_names() || lp.get_constraints().has_names()) {
            // OSI requires the objective name to be set whenever any variable or constraint names are set.
            lp_solver->setObjName("obj");
        }

        if (lp.get_variables().has_names() || lp.get_constraints().has_names() || !lp.get_objective_name().empty()) {
            lp_solver->setIntParam(OsiIntParam::OsiNameDiscipline, 2);
        } else {
            lp_solver->setIntParam(OsiIntParam::OsiNameDiscipline, 0);
        }

        if (lp.get_variables().has_names()) {
            for (int i = 0; i < lp.get_variables().size(); ++i) {
                lp_solver->setColName(i, lp.get_variables().get_name(i));
            }
        }

        if (lp.get_constraints().has_names()) {
            for (int i = 0; i < lp.get_constraints().size(); ++i) {
                lp_solver->setRowName(i, lp.get_constraints().get_name(i));
            }
        }
    } catch (CoinError &error) {
        handle_coin_error(error);
    }

    clear_temporary_data();
}

void LPSolver::add_temporary_constraints(const vector<LPConstraint> &constraints) {
    if (!constraints.empty()) {
        clear_temporary_data();
        int num_rows = constraints.size();
        for (const LPConstraint &constraint : constraints) {
            assert(utils::all_values_unique(constraint.get_variables()));
            row_lb.push_back(constraint.get_lower_bound());
            row_ub.push_back(constraint.get_upper_bound());
            rows.push_back(new CoinShallowPackedVector(
                               constraint.get_variables().size(),
                               constraint.get_variables().data(),
                               constraint.get_coefficients().data(),
                               false));
        }

        try {
            lp_solver->addRows(num_rows,
                               rows.data(), row_lb.data(), row_ub.data());
        } catch (CoinError &error) {
            handle_coin_error(error);
        }
        for (CoinPackedVectorBase *row : rows) {
            delete row;
        }
        clear_temporary_data();
        has_temporary_constraints_ = true;
        is_solved = false;
    }
}

void LPSolver::clear_temporary_constraints() {
    if (has_temporary_constraints_) {
        try {
            lp_solver->restoreBaseModel(num_permanent_constraints);
        } catch (CoinError &error) {
            handle_coin_error(error);
        }
        has_temporary_constraints_ = false;
        is_solved = false;
    }
}

double LPSolver::get_infinity() const {
    try {
        return lp_solver->getInfinity();
    } catch (CoinError &error) {
        handle_coin_error(error);
    }
}

void LPSolver::set_objective_coefficients(const vector<double> &coefficients) {
    assert(static_cast<int>(coefficients.size()) == get_num_variables());
    vector<int> indices(coefficients.size());
    iota(indices.begin(), indices.end(), 0);
    try {
        lp_solver->setObjCoeffSet(indices.data(),
                                  indices.data() + indices.size(),
                                  coefficients.data());
    } catch (CoinError &error) {
        handle_coin_error(error);
    }
    is_solved = false;
}

void LPSolver::set_objective_coefficient(int index, double coefficient) {
    assert(index < get_num_variables());
    try {
        lp_solver->setObjCoeff(index, coefficient);
    } catch (CoinError &error) {
        handle_coin_error(error);
    }
    is_solved = false;
}

void LPSolver::set_constraint_lower_bound(int index, double bound) {
    assert(index < get_num_constraints());
    try {
        lp_solver->setRowLower(index, bound);
    } catch (CoinError &error) {
        handle_coin_error(error);
    }
    is_solved = false;
}

void LPSolver::set_constraint_upper_bound(int index, double bound) {
    assert(index < get_num_constraints());
    try {
        lp_solver->setRowUpper(index, bound);
    } catch (CoinError &error) {
        handle_coin_error(error);
    }
    is_solved = false;
}

void LPSolver::set_variable_lower_bound(int index, double bound) {
    assert(index < get_num_variables());
    try {
        lp_solver->setColLower(index, bound);
    } catch (CoinError &error) {
        handle_coin_error(error);
    }
    is_solved = false;
}

void LPSolver::set_variable_upper_bound(int index, double bound) {
    assert(index < get_num_variables());
    try {
        lp_solver->setColUpper(index, bound);
    } catch (CoinError &error) {
        handle_coin_error(error);
    }
    is_solved = false;
}

void LPSolver::set_mip_gap(double gap) {
    lp::set_mip_gap(lp_solver.get(), gap);
}

void LPSolver::solve() {
    try {
        if (is_initialized) {
            lp_solver->resolve();
        } else {
            lp_solver->initialSolve();
            is_initialized = true;
        }
        if (is_mip) {
            lp_solver->branchAndBound();
        }
        if (lp_solver->isAbandoned()) {
            // The documentation of OSI is not very clear here but memory seems
            // to be the most common cause for this in our case.
            cerr << "Abandoned LP during resolve. "
                 << "Reasons include \"numerical difficulties\" and running out of memory." << endl;
            utils::exit_with(ExitCode::SEARCH_CRITICAL_ERROR);
        }
        is_solved = true;
    } catch (CoinError &error) {
        handle_coin_error(error);
    }
}

void LPSolver::write_lp(const string &filename) const {
    try {
        lp_solver->writeLp(filename.c_str());
    } catch (CoinError &error) {
        handle_coin_error(error);
    }
}

void LPSolver::print_failure_analysis() const {
    cout << "abandoned: " << lp_solver->isAbandoned() << endl;
    cout << "proven optimal: " << lp_solver->isProvenOptimal() << endl;
    cout << "proven primal infeasible: " << lp_solver->isProvenPrimalInfeasible() << endl;
    cout << "proven dual infeasible: " << lp_solver->isProvenDualInfeasible() << endl;
    cout << "dual objective limit reached: " << lp_solver->isDualObjectiveLimitReached() << endl;
    cout << "iteration limit reached: " << lp_solver->isIterationLimitReached() << endl;
}

bool LPSolver::has_optimal_solution() const {
    assert(is_solved);
    try {
        return !lp_solver->isProvenPrimalInfeasible() &&
               !lp_solver->isProvenDualInfeasible() &&
               lp_solver->isProvenOptimal();
    } catch (CoinError &error) {
        handle_coin_error(error);
    }
}

double LPSolver::get_objective_value() const {
    assert(has_optimal_solution());
    try {
        return lp_solver->getObjValue();
    } catch (CoinError &error) {
        handle_coin_error(error);
    }
}

bool LPSolver::is_infeasible() const {
    assert(is_solved);
    try {
        return lp_solver->isProvenPrimalInfeasible() &&
               !lp_solver->isProvenDualInfeasible() &&
               !lp_solver->isProvenOptimal();
    } catch (CoinError &error) {
        handle_coin_error(error);
    }
}

bool LPSolver::is_unbounded() const {
    assert(is_solved);
    try {
        return !lp_solver->isProvenPrimalInfeasible() &&
               lp_solver->isProvenDualInfeasible() &&
               !lp_solver->isProvenOptimal();
    } catch (CoinError &error) {
        handle_coin_error(error);
    }
}

vector<double> LPSolver::extract_solution() const {
    assert(has_optimal_solution());
    try {
        const double *sol = lp_solver->getColSolution();
        return vector<double>(sol, sol + get_num_variables());
    } catch (CoinError &error) {
        handle_coin_error(error);
    }
}

int LPSolver::get_num_variables() const {
    try {
        return lp_solver->getNumCols();
    } catch (CoinError &error) {
        handle_coin_error(error);
    }
}

int LPSolver::get_num_constraints() const {
    try {
        return lp_solver->getNumRows();
    } catch (CoinError &error) {
        handle_coin_error(error);
    }
}

int LPSolver::has_temporary_constraints() const {
    return has_temporary_constraints_;
}

void LPSolver::print_statistics() const {
    utils::g_log << "LP variables: " << get_num_variables() << endl;
    utils::g_log << "LP constraints: " << get_num_constraints() << endl;
}

#endif

static plugins::TypedEnumPlugin<LPSolverType> _enum_plugin({
        {"clp", "default LP solver shipped with the COIN library"},
        {"cplex", "commercial solver by IBM"},
        {"gurobi", "commercial solver"},
        {"soplex", "open source solver by ZIB"}
    });
}
