#ifndef LP_CPLEX_SOLVER_INTERFACE_H
#define LP_CPLEX_SOLVER_INTERFACE_H

#include "lp_solver.h"
#include "solver_interface.h"

#include "../algorithms/named_vector.h"

#include <cstring>
#include <cplex.h>

namespace lp {
template<typename T>
static T *to_cplex_array(std::vector<T> &v) {
    /*
      CPLEX expects a non-nullptr even for empty arrays but the C++ standard
      does not guarantee any particular value for data for empty vectors (see
      issue1111).
    */
    if (v.empty()) {
        static T dummy;
        return &dummy;
    } else {
        return v.data();
    }
}

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
      Matrix data in CPLEX format for loading a new problem. Matrix entries are
      stored in sparse form: non-zero coefficients are either stored
      column-by-column (in that case the column is the major dimension and the
      row is the minor dimension) or row-by-row (in that case the row is the
      major dimension and the column is the minor dimension).

      TODO: eventually, we might want to create our LP classes directly in this
      format, so no additional conversion is necessary.
    */
    class CplexMatrix {
        /*
          Non-zero entries in the matrix.
         */
        std::vector<double> coefficients;
        /*
          Parallel vector to coefficients: specifies the minor dimension of the
          corresponding entry.

          For example, say entries are stored column-by-column, and there are
          4 non-zero entries in columns 0 and 1. If column 2 now as a 4.5 in
          row 1 and a 7.2 in row 5, then
             coefficients[4] = 4.5, indices[4] = 1
             coefficients[5] = 7.2, indices[5] = 5.
         */
        std::vector<int> indices;
        /*
          Specifies the starts of the major dimension in the two vectors above.
          In the example above, starts[2] = 4 because the entries for column 2
          start at index 4.
         */
        std::vector<int> starts;
        /*
          Specifies the number of non-zero entries within a major dimension.
          In the example above, counts[2] = 2 because there are two non-zero
          entries for column 2 (4.5 and 7.2).
         */
        std::vector<int> counts;
public:
        /*
          When loading a whole LP, column-by-column data better matches CPLEX's
          internal data structures, so we prefer this encoding.
         */
        void assign_column_by_column(
            const named_vector::NamedVector<LPConstraint> &constraints,
            int num_cols);
        /*
          When adding constraints, a row-by-row encoding is more useful.
          In row form, counts are not used. Instead, starts has an additional
          entry at the end and counts[i] is always assumed to be
          (starts[i+1] - starts[i]).
         */
        void assign_row_by_row(
            const named_vector::NamedVector<LPConstraint> &constraints);

        double *get_coefficients() {return to_cplex_array(coefficients);}
        int *get_indices() {return to_cplex_array(indices);}
        int *get_starts() {return to_cplex_array(starts);}
        int *get_counts() {return to_cplex_array(counts);}
        int get_num_nonzeros() {return coefficients.size();}
    };

    class CplexColumnsInfo {
        // Lower bound for each column (variable)
        std::vector<double> lb;
        // Upper bound for each column (variable)
        std::vector<double> ub;
        // Type of each column (continuos, integer)
        std::vector<char> type;
        // Objective value of each column (variable)
        std::vector<double> objective;
public:
        void assign(const named_vector::NamedVector<LPVariable> &variables);
        double *get_lb() {return to_cplex_array(lb);}
        double *get_ub() {return to_cplex_array(ub);}
        char *get_type() {return to_cplex_array(type);}
        double *get_objective() {return to_cplex_array(objective);}
    };

    class CplexRowsInfo {
        // Right-hand side value of a row
        std::vector<double> rhs;
        // Sense of a row (Greater or equal, Less or equal, Equal, or Range)
        std::vector<char> sense;
        /*
          If the sense of a row is Range, then its value is restricted to the
          interval (RHS, RHS + range_value).
         */
        std::vector<double> range_values;
        /*
          In case not all rows specify a sense, this gives the indices of the
          rows that are ranged rows.
         */
        std::vector<int> range_indices;
public:
        void assign(const named_vector::NamedVector<LPConstraint> &constraints, int offset = 0, bool dense_range_values = true);
        double *get_rhs() {return to_cplex_array(rhs);}
        char *get_sense() {return to_cplex_array(sense);}
        double *get_range_values() {return to_cplex_array(range_values);}
        int *get_range_indices() {return to_cplex_array(range_indices);}
        int get_num_ranged_rows() {return range_indices.size();}
    };

    class CplexNameData {
        std::vector<char *> names;
        std::vector<int> indices;
public:
        template<typename T>
        explicit CplexNameData(const named_vector::NamedVector<T> &values) {
            if (values.has_names()) {
                names.reserve(values.size());
                indices.reserve(values.size());
                int num_values = values.size();
                for (int i = 0; i < num_values; ++i) {
                    const std::string &name = values.get_name(i);
                    if (!name.empty()) {
                        // CPLEX copies the names, so the const_cast should be fine.
                        names.push_back(const_cast<char *>(name.data()));
                        indices.push_back(i);
                    }
                }
            }
        }

        int size() {return names.size();}
        int *get_indices() {
            if (indices.empty()) {
                return nullptr;
            } else {
                return indices.data();
            }
        }
        char **get_names() {
            if (names.empty()) {
                return nullptr;
            } else {
                return names.data();
            }
        }
    };

    /*
      We could create these objects locally but we want to keep them around to
      avoid reallocation of the vectors in case we load multiple LPs. We don't
      do this for name data as names are expensive anyway and should only be
      used in debug mode where the additional allocation is not problematic.
     */
    CplexMatrix matrix;
    CplexColumnsInfo columns;
    CplexRowsInfo rows;
    std::vector<int> objective_indices;

    /*
      We store a copy of the current constraint bounds. We need to know the
      current bounds when changing bounds, and accessing them through the CPLEX
      interface has a significant overhead. Storing these vectors overlaps with
      storing CplexRowsInfo above. The difference is that CplexRowsInfo stores
      more information and we reuse it for temporary constraints, while we want
      to keep the following vectors always synchronized with the full LP
      (permanent and temporary constraints).
     */
    std::vector<double> constraint_lower_bounds;
    std::vector<double> constraint_upper_bounds;

    bool is_trivially_unsolvable() const;
    void change_constraint_bounds(int index, double lb, double ub);
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
