#ifndef LINEAR_PROGRAM_H
#define LINEAR_PROGRAM_H

#include "utilities.h"

#include <functional>
#include <vector>

#ifdef USE_LP
#define LP_METHOD(X) X;
#else
#define LP_METHOD(X) __attribute__((noreturn)) X {\
    ABORT("LP method called without USE_LP");\
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

class LPConstraint {
    std::vector<int> variables;
    std::vector<double> coefficients;
public:
    LPConstraint();
    ~LPConstraint();

    double lower_bound;
    double upper_bound;
    const std::vector<int> &get_variables() const {return variables; }
    const std::vector<double> &get_coefficients() const {return coefficients; }

    bool empty() const;
    void insert(int index, double coefficient);
};

struct LPVariable {
    double lower_bound;
    double upper_bound;
    double objective_coefficient;

    LPVariable(double lower_bound_, double upper_bound_, double objective_coefficient_);
    ~LPVariable();
};

class LP {
    bool is_initialized;
    bool is_solved;
    int num_permanent_constraints;
    bool has_temporary_constraints;
    OsiSolverInterface *lp_solver;

    /*
      All methods except the constructor only do something useful
      if the planner is compiled with USE_LP. Otherwise, the constructor
      just prints an error message and exits. All other methods should not
      be reachable this way so they just abort.
     */
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
    LP_METHOD(double translate_infinity(double value) const)
    template<typename T>
    LP_METHOD(double *build_array(const std::vector<T> &vec,
                  std::function<double(const T&)> func) const)
    LP_METHOD(CoinPackedVectorBase **get_rows(
                  const std::vector<LPConstraint> &constraints))
public:
    LP(LPSolverType solver_type);
    ~LP();

    LP_METHOD(void assign_problem(
                  LPObjectiveSense sense,
                  const std::vector<LPVariable> &variables,
                  const std::vector<LPConstraint> &constraints))
    LP_METHOD(int add_temporary_constraints(const std::vector<LPConstraint> &constraints))
    LP_METHOD(void clear_temporary_constraints())

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
