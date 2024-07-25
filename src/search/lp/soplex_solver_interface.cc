#include "soplex_solver_interface.h"

#include "lp_solver.h"

#include "../utils/system.h"

using namespace std;
using namespace soplex;

namespace lp {
static int get_obj_sense(LPObjectiveSense sense) {
    if (sense == LPObjectiveSense::MINIMIZE) {
        return SoPlex::OBJSENSE_MINIMIZE;
    } else {
        return SoPlex::OBJSENSE_MAXIMIZE;
    }
}

static LPRowSetReal constraints_to_row_set(const named_vector::NamedVector<LPConstraint> &constraints) {
    int num_rows = constraints.size();
    int num_nonzeros = 0;
    for (const LPConstraint &constraint : constraints) {
        num_nonzeros += constraint.get_coefficients().size();
    }

    LPRowSetReal rows(num_rows, num_nonzeros);
    for (const LPConstraint &constraint : constraints) {
        const vector<int> variables = constraint.get_variables();
        const vector<double> coefficients = constraint.get_coefficients();
        int num_entries = coefficients.size();
        soplex::DSVectorReal entries(num_entries);
        for (int i = 0; i < num_entries; ++i) {
            entries.add(variables[i], coefficients[i]);
        }
        rows.add(constraint.get_lower_bound(), entries, constraint.get_upper_bound());
    }
    return rows;
}

static LPColSetReal variables_to_col_set(const named_vector::NamedVector<LPVariable> &variables) {
    int num_cols = variables.size();
    LPColSetReal cols(num_cols, 0);
    DSVector emptycol(0);
    for (const LPVariable &var : variables) {
        cols.add(var.objective_coefficient, var.lower_bound, emptycol, var.upper_bound);
    }
    return cols;
}

SoPlexSolverInterface::SoPlexSolverInterface() : SolverInterface() {
    soplex.setIntParam(SoPlex::VERBOSITY, SoPlex::VERBOSITY_ERROR);
    soplex.setIntParam(SoPlex::SIMPLIFIER, SoPlex::SIMPLIFIER_OFF);
}

void SoPlexSolverInterface::load_problem(const LinearProgram &lp) {
    for (const LPVariable &var : lp.get_variables()) {
        if (var.is_integer) {
            cerr << "SoPlex does not support integer variables" << endl;
            utils::exit_with(utils::ExitCode::SEARCH_UNSUPPORTED);
        }
    }
    soplex.clearLPReal();
    soplex.setIntParam(SoPlex::OBJSENSE, get_obj_sense(lp.get_sense()));
    soplex.addColsReal(variables_to_col_set(lp.get_variables()));
    soplex.addRowsReal(constraints_to_row_set(lp.get_constraints()));
    num_permanent_constraints = lp.get_constraints().size();
    num_temporary_constraints = 0;
}

void SoPlexSolverInterface::add_temporary_constraints(const named_vector::NamedVector<LPConstraint> &constraints) {
    soplex.addRowsReal(constraints_to_row_set(constraints));
    num_temporary_constraints = constraints.size();
}

void SoPlexSolverInterface::clear_temporary_constraints() {
    if (has_temporary_constraints()) {
        int first = num_permanent_constraints;
        int last = first + num_temporary_constraints - 1;
        soplex.removeRowRangeReal(first, last, nullptr);
        num_temporary_constraints = 0;
    }
}

double SoPlexSolverInterface::get_infinity() const {
    return infinity;
}

void SoPlexSolverInterface::set_objective_coefficients(const vector<double> &coefficients) {
    int num_cols = coefficients.size();
    for (int i = 0; i < num_cols; ++i) {
        soplex.changeObjReal(i, coefficients[i]);
    }
}

void SoPlexSolverInterface::set_objective_coefficient(int index, double coefficient) {
    soplex.changeObjReal(index, coefficient);
}

void SoPlexSolverInterface::set_constraint_lower_bound(int index, double bound) {
    soplex.changeLhsReal(index, bound);
}

void SoPlexSolverInterface::set_constraint_upper_bound(int index, double bound) {
    soplex.changeRhsReal(index, bound);
}

void SoPlexSolverInterface::set_variable_lower_bound(int index, double bound) {
    soplex.changeLowerReal(index, bound);
}

void SoPlexSolverInterface::set_variable_upper_bound(int index, double bound) {
    soplex.changeUpperReal(index, bound);
}

void SoPlexSolverInterface::set_mip_gap(double /*gap*/) {
    /*
      There is nothing to do here: SoPlex doesn't accept MIPs, so setting a MIP
      gap tolerance has no effect. We do not treat it as an error, so solvers
      can be set up without checking what problems they will eventually solve.
      Loading a problem with integer variables will lead to an error either way.
     */
}

void SoPlexSolverInterface::solve() {
    soplex.optimize();
}

void SoPlexSolverInterface::write_lp(const string &filename) const {
    soplex.writeFile(filename.c_str());
}

void SoPlexSolverInterface::print_failure_analysis() const {
    cout << "SoPlex status: ";
    switch (soplex.status()) {
    case SPxSolverBase<double>::Status::ERROR:
        cout << "Error" << endl;
        break;
    case SPxSolverBase<double>::Status::NO_RATIOTESTER:
    case SPxSolverBase<double>::Status::NO_PRICER:
    case SPxSolverBase<double>::Status::NO_SOLVER:
        cout << "Missing component" << endl;
        break;
    case SPxSolverBase<double>::Status::NOT_INIT:
        cout << "Not initialized" << endl;
        break;
#if SOPLEX_VERSION < 700
    case SPxSolverBase<double>::Status::ABORT_EXDECOMP:
    case SPxSolverBase<double>::Status::ABORT_DECOMP:
#endif
    case SPxSolverBase<double>::Status::ABORT_CYCLING:
    case SPxSolverBase<double>::Status::ABORT_TIME:
    case SPxSolverBase<double>::Status::ABORT_ITER:
    case SPxSolverBase<double>::Status::ABORT_VALUE:
        cout << "Aborted" << endl;
        break;
    case SPxSolverBase<double>::Status::SINGULAR:
        cout << "Basis is singular, numerical troubles?" << endl;
        break;
    case SPxSolverBase<double>::Status::NO_PROBLEM:
        cout << "No Problem loaded" << endl;
        break;
    case SPxSolverBase<double>::Status::REGULAR:
        cout << "LP has a usable Basis (maybe LP is changed)." << endl;
        break;
    case SPxSolverBase<double>::Status::RUNNING:
        cout << "algorithm is running" << endl;
        break;
    case SPxSolverBase<double>::Status::UNKNOWN:
        cout << "nothing known on loaded problem." << endl;
        break;
    case SPxSolverBase<double>::Status::OPTIMAL:
        cout << "LP has been solved to optimality." << endl;
        break;
    case SPxSolverBase<double>::Status::UNBOUNDED:
        cout << "LP has been proven to be primal unbounded." << endl;
        break;
    case SPxSolverBase<double>::Status::INFEASIBLE:
        cout << "LP has been proven to be primal infeasible." << endl;
        break;
    case SPxSolverBase<double>::Status::INForUNBD:
        cout << "LP is primal infeasible or unbounded." << endl;
        break;
    case SPxSolverBase<double>::Status::OPTIMAL_UNSCALED_VIOLATIONS:
        cout << "LP has beed solved to optimality but unscaled solution contains violations." << endl;
        break;
    }
}

bool SoPlexSolverInterface::is_infeasible() const {
    assert(soplex.hasSol());
    return soplex.status() == SPxSolverBase<double>::Status::INFEASIBLE;
}

bool SoPlexSolverInterface::is_unbounded() const {
    assert(soplex.hasSol());
    return soplex.status() == SPxSolverBase<double>::Status::UNBOUNDED;
}

bool SoPlexSolverInterface::has_optimal_solution() const {
    assert(soplex.hasSol());
    return soplex.status() == SPxSolverBase<double>::Status::OPTIMAL;
}

double SoPlexSolverInterface::get_objective_value() const {
    assert(soplex.hasSol());
    return soplex.objValueReal();
}

vector<double> SoPlexSolverInterface::extract_solution() const {
    VectorBase<double> sol(get_num_variables());
    soplex.getPrimal(sol);
    return sol.vec();
}

int SoPlexSolverInterface::get_num_variables() const {
    return soplex.numCols();
}

int SoPlexSolverInterface::get_num_constraints() const {
    return num_permanent_constraints + num_temporary_constraints;
}

bool SoPlexSolverInterface::has_temporary_constraints() const {
    return num_temporary_constraints > 0;
}

void SoPlexSolverInterface::print_statistics() const {
    soplex.printStatistics(cout);
}
}
