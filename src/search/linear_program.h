#ifndef LINEAR_PROGRAM_H
#define LINEAR_PROGRAM_H

#include "utilities.h"

#include <functional>
#include <vector>

/*
  All methods that use COIN specific classes only do something useful
  if the planner is compiled with USE_LP. Otherwise, they just print
  an error message and abort.
 */
#ifdef USE_LP
#define LP_METHOD(X) X;
#else
#define LP_METHOD(X) __attribute__((noreturn)) X {\
    ABORT("LP method called without USE_LP. " \
          << "You must build the planner with the USE_LP symbol defined.");\
}
#endif

enum class LPSolverType {
    CLP, CPLEX, GUROBI
};

enum class LPObjectiveSense {
    MAXIMIZE, MINIMIZE
};

class CoinPackedVectorBase;
class OptionParser;
class OsiSolverInterface;

void add_lp_solver_option_to_parser(OptionParser &parser);

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"

class LPConstraint {
    std::vector<int> variables;
    std::vector<double> coefficients;
public:
    LP_METHOD(LPConstraint(double lower_bound_, double upper_bound_))
    LP_METHOD(~LPConstraint())

    double lower_bound;
    double upper_bound;
    LP_METHOD(CoinPackedVectorBase *create_coin_vector() const)

    LP_METHOD(bool empty() const)
    LP_METHOD(void insert(int index, double coefficient))
};

struct LPVariable {
    double lower_bound;
    double upper_bound;
    double objective_coefficient;

    LP_METHOD(LPVariable(double lower_bound_,
                         double upper_bound_,
                         double objective_coefficient_))
    LP_METHOD(~LPVariable())
};

class LP {
    bool is_initialized;
    bool is_solved;
    int num_permanent_constraints;
    bool has_temporary_constraints;
    OsiSolverInterface *lp_solver;

    template<typename T>
    LP_METHOD(double *build_array(const std::vector<T> &vec,
                  std::function<double(const T&)> func) const)
    LP_METHOD(CoinPackedVectorBase **create_rows(
                  const std::vector<LPConstraint> &constraints))
public:
    LP_METHOD(LP(LPSolverType solver_type))
    LP_METHOD(~LP())

    LP_METHOD(void assign_problem(
                  LPObjectiveSense sense,
                  const std::vector<LPVariable> &variables,
                  const std::vector<LPConstraint> &constraints))
    LP_METHOD(int add_temporary_constraints(const std::vector<LPConstraint> &constraints))
    LP_METHOD(void clear_temporary_constraints())
    LP_METHOD(double get_infinity())

    LP_METHOD(void set_objective_coefficient(int index, double coefficient))
    LP_METHOD(void set_constraint_lower_bound(int index, double bound))
    LP_METHOD(void set_constraint_upper_bound(int index, double bound))
    LP_METHOD(void set_variable_lower_bound(int index, double bound))
    LP_METHOD(void set_variable_upper_bound(int index, double bound))

    LP_METHOD(void solve())
    LP_METHOD(bool has_optimal_solution() const)
    LP_METHOD(double get_objective_value() const)
    LP_METHOD(std::vector<double> extract_solution() const)

    LP_METHOD(int get_num_variables() const)
    LP_METHOD(int get_num_constraints() const)
    LP_METHOD(void print_statistics() const)
#pragma GCC diagnostic pop
};

#endif
