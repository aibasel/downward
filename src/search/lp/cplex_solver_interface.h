#ifndef LP_CPLEX_SOLVER_INTERFACE_H
#define LP_CPLEX_SOLVER_INTERFACE_H
#ifdef HAS_CPLEX

#include "lp_solver.h"
#include "solver_interface.h"

#include "../algorithms/named_vector.h"
#include "../utils/memory.h"

#include <cplex.h>

namespace lp {
class CplexSolverInterface : public SolverInterface {
    CPXENVptr env;
    CPXLPptr problem;
    bool is_mip;
    int num_permanent_constraints;

    /*
      Our public interface allows using constraints of the form
        LB <= expression <= UB
      In cases where LB > UB, this constraint is trivially unsatisfiable.
      CPLEX does not represent constraints like this and instead uses range
      values, where the constraint is represented like this
        expression - RNG = LB
      where RNG is a variable restricted to take values from 0 to (UB - LB).
      If LB > UB, the semantic instead is that RNG takes negative values between
      (UB - LB) and 0. This means that in CPLEX, the constraint never is
      trivially unsolvable. We still set the range value and the right-hand side
      as described above but use negative range values to represent trivially
      unsatisfiable constraints. The following two counters track how many such
      constraints we have in the permanent and the temporary constraints.
    */
    int num_unsatisfiable_constraints;
    int num_unsatisfiable_temp_constraints;

    /*
      Temporary data for assigning a new problem. We keep the vectors
      around to avoid recreating them in every assignment.
    */
    std::vector<double> elements;
    std::vector<int> indices;
    std::vector<int> starts;
    std::vector<double> col_lb;
    std::vector<double> col_ub;
    std::vector<char> col_type;
    std::vector<double> objective;
    std::vector<double> row_rhs;
    std::vector<char> row_sense;
    std::vector<double> row_range_values;
    std::vector<int> row_range_indices;
    void clear_temporary_data();

    void add_variables(const named_vector::NamedVector<LPVariable> &variables);
    void add_constraints(const named_vector::NamedVector<LPConstraint> &constraints, bool temporary);
    std::pair<double, double> get_constraint_bounds(int index);
    bool is_trivially_unsolvable() const;
    void change_constraint_bounds(int index, double current_lb, double current_ub,
                                  double lb, double ub);
public:
    CplexSolverInterface();
    virtual ~CplexSolverInterface() override;

    virtual void load_problem(const LinearProgram &lp) override;
    virtual void add_temporary_constraints(const named_vector::NamedVector<LPConstraint> &constraints) override;
    virtual void clear_temporary_constraints() override;
    virtual double get_infinity() const override;
    virtual void set_objective_coefficients(const std::vector<double> &coefficients) override;
    virtual void set_objective_coefficient(int index, double coefficient) override;
    virtual void set_constraint_lower_bound(int index, double bound) override;
    virtual void set_constraint_upper_bound(int index, double bound) override;
    virtual void set_variable_lower_bound(int index, double bound) override;
    virtual void set_variable_upper_bound(int index, double bound) override;
    virtual void set_mip_gap(double gap) override;
    virtual void solve() override;
    virtual void write_lp(const std::string &filename) const override;
    virtual void print_failure_analysis() const override;
    virtual bool is_infeasible() const override;
    virtual bool is_unbounded() const override;
    virtual bool has_optimal_solution() const override;
    virtual double get_objective_value() const override;
    virtual std::vector<double> extract_solution() const override;
    virtual int get_num_variables() const override;
    virtual int get_num_constraints() const override;
    virtual bool has_temporary_constraints() const override;
    virtual void print_statistics() const override;
};
}
#endif
#endif
