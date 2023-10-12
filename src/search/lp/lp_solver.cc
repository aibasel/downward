#include "lp_solver.h"

#ifdef HAS_CPLEX
#include "cplex_solver_interface.h"
#endif
#ifdef HAS_SOPLEX
#include "soplex_solver_interface.h"
#endif

#include "../plugins/plugin.h"

using namespace std;

namespace lp {
void add_lp_solver_option_to_feature(plugins::Feature &feature) {
    feature.add_option<LPSolverType>(
        "lpsolver",
        "external solver that should be used to solve linear programs",
        "cplex");

    feature.document_note(
        "Note",
        "to use an LP solver, you must build the planner with LP support. "
        "See [build instructions https://github.com/aibasel/downward/blob/main/BUILD.md].");
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

ostream &LPConstraint::dump(ostream &stream, const LinearProgram *program) const {
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


LPSolver::LPSolver(LPSolverType solver_type) {
    string missing_solver;
    switch (solver_type) {
    case LPSolverType::CPLEX:
#ifdef HAS_CPLEX
        pimpl = make_unique<CplexSolverInterface>();
#else
        missing_solver = "CPLEX";
#endif
        break;
    case LPSolverType::SOPLEX:
#ifdef HAS_SOPLEX
        pimpl = make_unique<SoPlexSolverInterface>();
#else
        missing_solver = "SoPlex";
#endif
        break;
    default:
        ABORT("Unknown LP solver type.");
    }
    if (!pimpl) {
        cerr << "Tried to use LP solver " << missing_solver
             << ", but the planner was compiled without support for it."
             << endl
             << "See https://github.com/aibasel/downward/blob/main/BUILD.md\n"
             << "to install " << missing_solver
             << " and use it in the planner." << endl;
        utils::exit_with(utils::ExitCode::SEARCH_CRITICAL_ERROR);
    }
}

void LPSolver::load_problem(const LinearProgram &lp) {
    pimpl->load_problem(lp);
}

void LPSolver::add_temporary_constraints(const named_vector::NamedVector<LPConstraint> &constraints) {
    pimpl->add_temporary_constraints(constraints);
}

void LPSolver::clear_temporary_constraints() {
    pimpl->clear_temporary_constraints();
}

double LPSolver::get_infinity() const {
    return pimpl->get_infinity();
}

void LPSolver::set_objective_coefficients(const vector<double> &coefficients) {
    pimpl->set_objective_coefficients(coefficients);
}

void LPSolver::set_objective_coefficient(int index, double coefficient) {
    pimpl->set_objective_coefficient(index, coefficient);
}

void LPSolver::set_constraint_lower_bound(int index, double bound) {
    pimpl->set_constraint_lower_bound(index, bound);
}

void LPSolver::set_constraint_upper_bound(int index, double bound) {
    pimpl->set_constraint_upper_bound(index, bound);
}

void LPSolver::set_variable_lower_bound(int index, double bound) {
    pimpl->set_variable_lower_bound(index, bound);
}

void LPSolver::set_variable_upper_bound(int index, double bound) {
    pimpl->set_variable_upper_bound(index, bound);
}

void LPSolver::set_mip_gap(double gap) {
    pimpl->set_mip_gap(gap);
}

void LPSolver::solve() {
    pimpl->solve();
}

void LPSolver::write_lp(const string &filename) const {
    pimpl->write_lp(filename);
}

void LPSolver::print_failure_analysis() const {
    pimpl->print_failure_analysis();
}

bool LPSolver::has_optimal_solution() const {
    return pimpl->has_optimal_solution();
}

double LPSolver::get_objective_value() const {
    return pimpl->get_objective_value();
}

bool LPSolver::is_infeasible() const {
    return pimpl->is_infeasible();
}

bool LPSolver::is_unbounded() const {
    return pimpl->is_unbounded();
}

vector<double> LPSolver::extract_solution() const {
    return pimpl->extract_solution();
}

int LPSolver::get_num_variables() const {
    return pimpl->get_num_variables();
}

int LPSolver::get_num_constraints() const {
    return pimpl->get_num_constraints();
}

int LPSolver::has_temporary_constraints() const {
    return pimpl->has_temporary_constraints();
}

void LPSolver::print_statistics() const {
    pimpl->print_statistics();
}

static plugins::TypedEnumPlugin<LPSolverType> _enum_plugin({
        {"cplex", "commercial solver by IBM"},
        {"soplex", "open source solver by ZIB"}
    });
}
