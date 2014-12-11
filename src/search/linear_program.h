#ifndef LINEAR_PROGRAM_H
#define LINEAR_PROGRAM_H

// TODO: merge with lp_solver_interface.h?
#include "lp_solver_interface.h"
#include "utilities.h"

#include <vector>

#ifdef USE_LP
#define LP_METHOD(X) X;
#else
#define LP_METHOD(X) __attribute__((noreturn)) X {\
    ABORT("LP method called without USE_LP");\
}
#endif

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

class OsiSolverInterface;

class LP {
    bool is_initialized;
    bool is_solved;
    int num_permanent_constraints;
    bool has_temporary_constraints;

#ifdef USE_LP
    OsiSolverInterface *lp_solver;
    double translate_infinity(double value);
    template<typename T>
    double *get_lower_bounds(const std::vector<T> &collection);
    template<typename T>
    double *get_upper_bounds(const std::vector<T> &collection);
    double *get_objectives(const std::vector<LPVariable> &variables);
    CoinPackedVectorBase **get_rows(const std::vector<LPConstraint> &constraints);
#endif
public:
    /*
      All public methods except the constructor only do something useful
      if the planner is compiled with USE_LP. Otherwise, the constructor
      just prints an error message and exits. All other methods should not
      be reachable this way so they just abort.
     */
    LP(LPSolverType solver_type,
       const std::vector<LPVariable> &variables,
       const std::vector<LPConstraint> &constraints,
       LPObjectiveSense sense);
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
    LP_METHOD(~LP())

    LP_METHOD(int add_temporary_constraints(const std::vector<LPConstraint> &constraints))
    LP_METHOD(void remove_temporary_constraints())

    LP_METHOD(void set_objective_coefficient(int index, double coefficient))
    LP_METHOD(void set_constraint_lower_bound(int index, double bound))
    LP_METHOD(void set_constraint_upper_bound(int index, double bound))
    LP_METHOD(void set_variable_lower_bound(int index, double bound))
    LP_METHOD(void set_variable_upper_bound(int index, double bound))

    LP_METHOD(void solve())
    LP_METHOD(bool has_optimal_solution() const)
    LP_METHOD(double get_objective_value() const)
    LP_METHOD(void extract_solution(std::vector<double> &solution) const)

    LP_METHOD(int get_num_variables() const)
    LP_METHOD(int get_num_constraints() const)
    LP_METHOD(void print_statistics() const)
#pragma GCC diagnostic pop
};

#endif
