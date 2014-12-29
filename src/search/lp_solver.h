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
    ABORT("LP method called but the planner was compiled without LP support." \
          << std::endl \
          << "See http://www.fast-downward.org/LPBuildInstructions " \
          << std::endl \
          << "to install an LP solver and use it in the planner.");\
}
#endif

enum class LpSolverType {
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

class LpConstraint {
    std::vector<int> variables;
    std::vector<double> coefficients;
public:
    LpConstraint(double lower_bound_, double upper_bound_);
    ~LpConstraint();

    double lower_bound;
    double upper_bound;
    const std::vector<int> &get_variables() const {return variables; }
    const std::vector<double> &get_coefficients() const {return coefficients; }

    void clear();
    bool empty() const;
    // Coefficients must be added ordered by index, without duplicate indices.
    void insert(int index, double coefficient);
};

struct LpVariable {
    double lower_bound;
    double upper_bound;
    double objective_coefficient;

    LP_METHOD(LpVariable(double lower_bound_,
                         double upper_bound_,
                         double objective_coefficient_))
    LP_METHOD(~LpVariable())
};

class LpSolver {
    bool is_initialized;
    bool is_solved;
    int num_permanent_constraints;
    bool has_temporary_constraints;
    OsiSolverInterface *lp_solver;

    /*
      Temporary data for assigning a new problem. We keep the vectors
      around to avoid recreating them in every assignment.
     */
    std::vector<double> elements;
    std::vector<int> indices;
    std::vector<int> starts;
    std::vector<double> col_lb;
    std::vector<double> col_ub;
    std::vector<double> objective;
    std::vector<double> row_lb;
    std::vector<double> row_ub;
    std::vector<CoinPackedVectorBase *> rows;
    void clear_temporary_data();

    template<typename T>
    LP_METHOD(double *build_array(const std::vector<T> &vec,
                  std::function<double(const T&)> func) const)
    LP_METHOD(CoinPackedVectorBase **create_rows(
                  const std::vector<LpConstraint> &constraints))
public:
    LP_METHOD(explicit LpSolver(LpSolverType solver_type))
    LP_METHOD(~LpSolver())

    LP_METHOD(void load_problem(
                  LPObjectiveSense sense,
                  const std::vector<LpVariable> &variables,
                  const std::vector<LpConstraint> &constraints))
    LP_METHOD(void add_temporary_constraints(const std::vector<LpConstraint> &constraints))
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
