#ifndef LINEAR_PROGRAM_H
#define LINEAR_PROGRAM_H

// TODO: merge with lp_solver_interface.h?
#include "lp_solver_interface.h"

#ifdef USE_LP
#pragma GCC diagnostic ignored "-Wunused-parameter"
#include <OsiSolverInterface.hpp>
#endif

#include <vector>

enum LPObjectiveSense {
    MAXIMIZE, MINIMIZE
};

class LPConstraint {
    std::vector<int> indices;
    std::vector<double> coefficients;
public:
    LPConstraint();
    ~LPConstraint();

    double lower_bound;
    double upper_bound;
    const std::vector<int> &get_indices() const {return indices; }
    const std::vector<double> &get_coefficients() const {return coefficients; }

    bool empty() const;
    void insert(int index, double coefficient);
};

struct LPVariable {
    double lower_bound;
    double upper_bound;
    double objective;

    LPVariable(double lower_bound_, double upper_bound_, double objective_)
        : lower_bound(lower_bound_),
          upper_bound(upper_bound_),
          objective(objective_) {
    }
    ~LPVariable() {
    }
};

class LP {
    bool is_solved;
    int num_permanent_constraints;
    bool has_temporary_constraints;

#ifdef USE_LP
    OsiSolverInterface *lp_solver;
#endif
    double translate_infinity(double value);
    template<typename T>
    double *get_lower_bounds(const std::vector<T> &collection);
    template<typename T>
    double *get_upper_bounds(const std::vector<T> &collection);
    double *get_objectives(const std::vector<LPVariable> &variables);
#ifdef USE_LP
    CoinPackedVectorBase **get_rows(const std::vector<LPConstraint> &constraints);
#endif
public:
    LP(LPSolverType solver_type,
       const std::vector<LPVariable> &variables,
       const std::vector<LPConstraint> &constraints,
       LPObjectiveSense sense);
    ~LP();

    int add_temporary_constraints(const std::vector<LPConstraint> &constraints);
    void remove_temporary_constraints();

    void set_objective_coefficient(int index, double coefficient);
    void set_constraint_lower_bound(int index, double bound);
    void set_constraint_upper_bound(int index, double bound);
    void set_variable_lower_bound(int index, double bound);
    void set_variable_upper_bound(int index, double bound);

    void solve();
    bool has_optimal_solution() const;
    double objective_value() const;
    void extract_solution(std::vector<double> &solution) const;

    int get_num_variables() const;
    int get_num_constraints() const;
    void print_statistics() const;
};

#endif
