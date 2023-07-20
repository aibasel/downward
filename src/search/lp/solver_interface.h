#ifndef LP_SOLVER_INTERFACE_H
#define LP_SOLVER_INTERFACE_H

#include <string>
#include <vector>

namespace named_vector {
template<typename T>
class NamedVector;
}

namespace lp {
class LinearProgram;
class LPConstraint;

class SolverInterface {
public:
    virtual ~SolverInterface() = default;

    virtual void load_problem(const LinearProgram &lp) = 0;
    virtual void add_temporary_constraints(const named_vector::NamedVector<LPConstraint> &constraints) = 0;
    virtual void clear_temporary_constraints() = 0;
    virtual double get_infinity() const = 0;

    virtual void set_objective_coefficients(const std::vector<double> &coefficients) = 0;
    virtual void set_objective_coefficient(int index, double coefficient) = 0;
    virtual void set_constraint_lower_bound(int index, double bound) = 0;
    virtual void set_constraint_upper_bound(int index, double bound) = 0;
    virtual void set_variable_lower_bound(int index, double bound) = 0;
    virtual void set_variable_upper_bound(int index, double bound) = 0;

    virtual void set_mip_gap(double gap) = 0;

    virtual void solve() = 0;
    virtual void write_lp(const std::string &filename) const = 0;
    virtual void print_failure_analysis() const = 0;
    virtual bool is_infeasible() const = 0;
    virtual bool is_unbounded() const = 0;

    /*
      Return true if the solving the LP showed that it is bounded feasible and
      the discovered solution is guaranteed to be optimal. We test for
      optimality explicitly because solving the LP sometimes finds suboptimal
      solutions due to numerical difficulties.
      The LP has to be solved with a call to solve() before calling this method.
    */
    virtual bool has_optimal_solution() const = 0;

    /*
      Return the objective value found after solving an LP.
      The LP has to be solved with a call to solve() and has to have an optimal
      solution before calling this method.
    */
    virtual double get_objective_value() const = 0;

    /*
      Return the solution found after solving an LP as a vector with one entry
      per variable.
      The LP has to be solved with a call to solve() and has to have an optimal
      solution before calling this method.
    */
    virtual std::vector<double> extract_solution() const = 0;

    virtual int get_num_variables() const = 0;
    virtual int get_num_constraints() const = 0;
    virtual bool has_temporary_constraints() const = 0;
    virtual void print_statistics() const = 0;
};
}

#endif
