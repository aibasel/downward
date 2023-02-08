#include "soplex_solver_interface.h"

namespace lp {
void SoPlexSolverInterface::load_problem(const LinearProgram &/*lp*/) {}
void SoPlexSolverInterface::add_temporary_constraints(const std::vector<LPConstraint> &/*constraints*/) {}
void SoPlexSolverInterface::clear_temporary_constraints() {}
double SoPlexSolverInterface::get_infinity() const {return 0.0;}

void SoPlexSolverInterface::set_objective_coefficients(const std::vector<double> &/*coefficients*/) {}
void SoPlexSolverInterface::set_objective_coefficient(int /*index*/, double /*coefficient*/) {}
void SoPlexSolverInterface::set_constraint_lower_bound(int /*index*/, double /*bound*/) {}
void SoPlexSolverInterface::set_constraint_upper_bound(int /*index*/, double /*bound*/) {}
void SoPlexSolverInterface::set_variable_lower_bound(int /*index*/, double /*bound*/) {}
void SoPlexSolverInterface::set_variable_upper_bound(int /*index*/, double /*bound*/) {}

void SoPlexSolverInterface::set_mip_gap(double /*gap*/) {}

void SoPlexSolverInterface::solve() {}
void SoPlexSolverInterface::write_lp(const std::string &/*filename*/) const {}
void SoPlexSolverInterface::print_failure_analysis() const {}
bool SoPlexSolverInterface::is_infeasible() const {return false;}
bool SoPlexSolverInterface::is_unbounded() const {return false;}

bool SoPlexSolverInterface::has_optimal_solution() const {return false;}

double SoPlexSolverInterface::get_objective_value() const {return 0.0;}

std::vector<double> SoPlexSolverInterface::extract_solution() const {return std::vector<double>();}

int SoPlexSolverInterface::get_num_variables() const {return 0;}
int SoPlexSolverInterface::get_num_constraints() const {return 0;}
int SoPlexSolverInterface::has_temporary_constraints() const {return 0;}
void SoPlexSolverInterface::print_statistics() const {}
}
