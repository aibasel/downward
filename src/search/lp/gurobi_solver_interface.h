#ifndef LP_GUROBI_SOLVER_INTERFACE_H
#define LP_GUROBI_SOLVER_INTERFACE_H

#include "solver_interface.h"

#include <gurobi_c.h>

namespace lp {
class GurobiSolverInterface : public SolverInterface {
    GRBenv *env;
    GRBmodel *model;
    int num_permanent_constraints;
    int num_temporary_constraints;
    bool model_dirty;

public:
    GurobiSolverInterface();
    virtual ~GurobiSolverInterface() override;

    virtual void load_problem(const LinearProgram &lp) override;
    virtual void add_temporary_constraints(
        const named_vector::NamedVector<LPConstraint> &constraints) override;
    virtual void clear_temporary_constraints() override;
    virtual double get_infinity() const override;

    virtual void set_objective_coefficients(
        const std::vector<double> &coefficients) override;
    virtual void set_objective_coefficient(int index, double coefficient) override;
    virtual void set_constraint_rhs(int index, double right_hand_side) override;
    virtual void set_constraint_sense(int index, lp::Sense sense) override;
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
