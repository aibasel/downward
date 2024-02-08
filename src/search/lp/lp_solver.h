#ifndef LP_LP_SOLVER_H
#define LP_LP_SOLVER_H

#include "solver_interface.h"

#include "../algorithms/named_vector.h"

#include <iostream>
#include <memory>
#include <vector>


namespace plugins {
class Feature;
class Options;
}

namespace lp {
enum class LPSolverType {
    CPLEX, SOPLEX
};

enum class LPObjectiveSense {
    MAXIMIZE, MINIMIZE
};

void add_lp_solver_option_to_feature(plugins::Feature &feature);
std::tuple<LPSolverType> get_lp_solver_arguments_from_options(
    const plugins::Options &opts);

class LinearProgram;

class LPConstraint {
    std::vector<int> variables;
    std::vector<double> coefficients;
    double lower_bound;
    double upper_bound;
public:
    LPConstraint(double lower_bound, double upper_bound);

    const std::vector<int> &get_variables() const {return variables;}
    const std::vector<double> &get_coefficients() const {return coefficients;}

    double get_lower_bound() const {return lower_bound;}
    void set_lower_bound(double lb) {lower_bound = lb;}
    double get_upper_bound() const {return upper_bound;}
    void set_upper_bound(double ub) {upper_bound = ub;}

    void clear();
    bool empty() const;
    // Coefficients must be added without duplicate indices.
    void insert(int index, double coefficient);

    std::ostream &dump(std::ostream &stream, const LinearProgram *program = nullptr) const;
};

struct LPVariable {
    double lower_bound;
    double upper_bound;
    double objective_coefficient;
    bool is_integer;

    LPVariable(double lower_bound,
               double upper_bound,
               double objective_coefficient,
               bool is_integer = false);
};

class LinearProgram {
    LPObjectiveSense sense;
    std::string objective_name;

    named_vector::NamedVector<LPVariable> variables;
    named_vector::NamedVector<LPConstraint> constraints;
    double infinity;

public:
    // objective_name is the name of the objective function used when writing the lp to a file.
    LinearProgram(LPObjectiveSense sense,
                  named_vector::NamedVector<LPVariable> &&variables,
                  named_vector::NamedVector<LPConstraint> &&constraints,
                  double infinity)
        : sense(sense), variables(std::move(variables)),
          constraints(std::move(constraints)), infinity(infinity) {
    }

    /*
      Variables and constraints can be given a custom name for debugging purposes.
      This has an impact on performance and should not be used in production code.
     */
    named_vector::NamedVector<LPVariable> &get_variables();
    named_vector::NamedVector<LPConstraint> &get_constraints();
    const named_vector::NamedVector<LPVariable> &get_variables() const;
    const named_vector::NamedVector<LPConstraint> &get_constraints() const;
    double get_infinity() const;
    LPObjectiveSense get_sense() const;
    void set_objective_name(const std::string &name);
    const std::string &get_objective_name() const;
};

class LPSolver {
    std::unique_ptr<SolverInterface> pimpl;
public:
    explicit LPSolver(LPSolverType solver_type);

    void load_problem(const LinearProgram &lp);
    void add_temporary_constraints(const named_vector::NamedVector<LPConstraint> &constraints);
    void clear_temporary_constraints();
    double get_infinity() const;

    void set_objective_coefficients(const std::vector<double> &coefficients);
    void set_objective_coefficient(int index, double coefficient);
    void set_constraint_lower_bound(int index, double bound);
    void set_constraint_upper_bound(int index, double bound);
    void set_variable_lower_bound(int index, double bound);
    void set_variable_upper_bound(int index, double bound);

    void set_mip_gap(double gap);

    void solve();
    void write_lp(const std::string &filename) const;
    void print_failure_analysis() const;
    bool is_infeasible() const;
    bool is_unbounded() const;

    /*
      Return true if the solving the LP showed that it is bounded feasible and
      the discovered solution is guaranteed to be optimal. We test for
      optimality explicitly because solving the LP sometimes finds suboptimal
      solutions due to numerical difficulties.
      The LP has to be solved with a call to solve() before calling this method.
    */
    bool has_optimal_solution() const;

    /*
      Return the objective value found after solving an LP.
      The LP has to be solved with a call to solve() and has to have an optimal
      solution before calling this method.
    */
    double get_objective_value() const;

    /*
      Return the solution found after solving an LP as a vector with one entry
      per variable.
      The LP has to be solved with a call to solve() and has to have an optimal
      solution before calling this method.
    */
    std::vector<double> extract_solution() const;

    int get_num_variables() const;
    int get_num_constraints() const;
    int has_temporary_constraints() const;
    void print_statistics() const;
};
}

#endif
