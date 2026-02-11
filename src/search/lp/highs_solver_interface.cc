#include "highs_solver_interface.h"

#include <cassert>
#include <iostream>
#include <stdexcept>

using namespace std;

namespace lp {

namespace {
struct RowBounds {
    double lower;
    double upper;
};

bool is_neg_inf(double value, double inf) {
    return value <= -inf;
}

bool is_pos_inf(double value, double inf) {
    return value >= inf;
}

RowBounds sense_rhs_to_bounds(const Highs &highs, Sense sense, double rhs) {
    const double inf = highs.getInfinity();
    switch (sense) {
        case Sense::GE:
            return {rhs, inf};
        case Sense::LE:
            return {-inf, rhs};
        case Sense::EQ:
            return {rhs, rhs};
    }
    throw std::runtime_error("Unknown constraint sense in HiGHS interface");
}

Sense bounds_to_sense(double lb, double ub, double inf) {
    const bool lb_inf = is_neg_inf(lb, inf);
    const bool ub_inf = is_pos_inf(ub, inf);

    if (lb_inf && !ub_inf) {
        return Sense::LE;
    }
    if (!lb_inf && ub_inf) {
        return Sense::GE;
    }
    if (!lb_inf && !ub_inf && lb == ub) {
        return Sense::EQ;
    }
    throw std::runtime_error("HiGHS constraint has unsupported ranged bounds");
}

double bounds_to_rhs(double lb, double ub, double inf) {
    const bool lb_inf = is_neg_inf(lb, inf);
    const bool ub_inf = is_pos_inf(ub, inf);

    if (lb_inf && !ub_inf) {
        return ub;
    }
    if (!lb_inf && ub_inf) {
        return lb;
    }
    if (!lb_inf && !ub_inf && lb == ub) {
        return lb;
    }
    throw std::runtime_error("HiGHS constraint has unsupported ranged bounds");
}
} // namespace

static void highs_ok(HighsStatus s, const char* where) {
    if (s == HighsStatus::kOk) return;
    throw std::runtime_error(std::string("HiGHS error in ") + where);
}

HiGHSSolverInterface::HiGHSSolverInterface() {
    // Nothing to do here :)
}

void HiGHSSolverInterface::load_problem(const LinearProgram &lp) {
    highs_ok(highs_.clear(), "clear");
    if (lp.get_sense() == LPObjectiveSense::MAXIMIZE){
        highs_ok(highs_.changeObjectiveSense(ObjSense::kMaximize), "changeObjectiveSense");
    }
    else{
        highs_ok(highs_.changeObjectiveSense(ObjSense::kMinimize), "changeObjectiveSense");
    }

    highs_ok(highs_.setOptionValue("output_flag", false), "setOptionValue(output_flag)");
    highs_ok(highs_.setOptionValue("log_to_console", false), "setOptionValue(log_to_console)");

    // variables
    const auto &vars = lp.get_variables();
    const int n = static_cast<int>(vars.size());
    for (int i = 0; i < n; ++i) {
        const auto &v = vars[i];
        highs_ok(highs_.addCol(v.objective_coefficient,
                               v.lower_bound, v.upper_bound,
                               0, nullptr, nullptr),
                 "addCol");

        if (v.is_integer) {
            highs_ok(highs_.changeColIntegrality(i, HighsVarType::kInteger),
                     "changeColIntegrality");
        }
    } 

    // constraints
    const auto &cons = lp.get_constraints();
    const int m = static_cast<int>(cons.size());
    for (int r = 0; r < m; ++r) {
        const auto &c = cons[r];
        const auto &idx = c.get_variables();
        const auto &val = c.get_coefficients();
        assert(idx.size() == val.size());

        const RowBounds bounds = sense_rhs_to_bounds(highs_, c.get_sense(), c.get_right_hand_side());
        highs_ok(highs_.addRow(bounds.lower,
                               bounds.upper,
                               (int)idx.size(),
                               idx.data(),
                               val.data()),
                 "addRow");
    }
    num_permanent_constraints = lp.get_constraints().size();
    num_temporary_constraints = 0;

    // Print model 
    //cerr << "Model loaded with " << n << " variables and " << m << " constraints." << endl;
    //for (int i = 0; i < n; ++i) {
    //    cerr << "Variable " << i << ": obj=" << vars[i].objective_coefficient
    //         << ", lb=" << vars[i].lower_bound
    //         << ", ub=" << vars[i].upper_bound
    //         << ", is_integer=" << vars[i].is_integer
    //         << endl;
    //}
    // for (int r = 0; r < m; ++r) {
    //    const auto &c = cons[r];
    //    cerr << "Constraint " << r << ": sense=" 
    //         << (c.get_sense() == lp::Sense::GE ? "GE" : (c.get_sense() == lp::Sense::LE ? "LE" : (c.get_sense() == lp::Sense::EQ ? "EQ" : "UNKNOWN"))) 
    //         << ", rhs=" << c.get_right_hand_side()
    //         << ", coefficients=[";
    //    for (size_t i = 0; i < c.get_variables().size(); ++i) {
    //        cerr << "(" << c.get_variables()[i] << ": " << c.get_coefficients()[i] << ")";
    //        if (i + 1 < c.get_variables().size()) {
    //            cerr << ", ";
    //        }
    //    }
    //    cerr << "]" << endl;
    //}
}

void HiGHSSolverInterface::add_temporary_constraints(
    const named_vector::NamedVector<LPConstraint> &constraints) {
    num_temporary_constraints += constraints.size();

    for (int i = 0; i < (int)constraints.size(); ++i) {
        const auto &c = constraints[i];
        const auto &idx = c.get_variables();
        const auto &val = c.get_coefficients();

        const RowBounds bounds = sense_rhs_to_bounds(highs_, c.get_sense(), c.get_right_hand_side());

        highs_ok(highs_.addRow(bounds.lower,
                               bounds.upper,
                               (int)idx.size(),
                               idx.data(),
                               val.data()),
                 "addRow(temporary_constraints)");
    }

    //cerr << "@@@@@@@@@@@@@@@@@@@@@ Warning: add_temporary_constraints does not update the constraint right-hand side and sense. Make sure to call set_constraint_rhs and set_constraint_sense if the right-hand side or sense should be changed." << endl;
   // exit(1);
}

void HiGHSSolverInterface::clear_temporary_constraints() {
    if (num_temporary_constraints == 0)
        return;

    const int fst_temp_row = num_permanent_constraints;
    const int lst_temp_row = highs_.getNumRow() - 1;
    highs_ok(highs_.deleteRows(fst_temp_row, lst_temp_row), "deleteRows");

    num_temporary_constraints = 0;
}

double HiGHSSolverInterface::get_infinity() const {
    return highs_.getInfinity();
    //return 1e100;
}

void HiGHSSolverInterface::set_objective_coefficients(const std::vector<double>& coefficients)
{
    const int n = highs_.getNumCol();

    if (coefficients.size() != static_cast<size_t>(n)) {
        throw std::runtime_error(
            "set_objective_coefficients size mismatch.\nExpected " +
            std::to_string(n) +
            " but instead got " +
            std::to_string(coefficients.size())
        );
    }

    for (int i = 0; i < n; ++i) {
        highs_ok(highs_.changeColCost(i, coefficients[i]),
                 "changeColCost(vec)");
    }

    //cerr << ">>>>> New objective coefficients: [";
    //for (int i = 0; i < n; ++i) {
    //    cerr << coefficients[i];
    //    if (i + 1 < n) {
    //        cerr << ", ";
    //    }
    //}
    //cerr << "]" << endl;
}

void HiGHSSolverInterface::set_objective_coefficient(int index, double coefficient) {
    highs_ok(highs_.changeColCost(index, coefficient), "changeColCost");
    //cerr << ">>>>> New objective coefficient for variable " << index << ": " << coefficient << endl;
}

void HiGHSSolverInterface::set_constraint_rhs(int index, double right_hand_side) {
    const double inf = highs_.getInfinity();
    const double lb = highs_.getLp().row_lower_[index];
    const double ub = highs_.getLp().row_upper_[index];
    const Sense sense = bounds_to_sense(lb, ub, inf);
    const RowBounds bounds = sense_rhs_to_bounds(highs_, sense, right_hand_side);
    highs_ok(highs_.changeRowBounds(index, bounds.lower, bounds.upper), "changeRowBounds(rhs)");
    //cerr << ">>>>> New rhs for constraint " << index << ": " << right_hand_side << endl;
}

void HiGHSSolverInterface::set_constraint_sense(int index, lp::Sense sense) {
    const double inf = highs_.getInfinity();
    const double lb = highs_.getLp().row_lower_[index];
    const double ub = highs_.getLp().row_upper_[index];
    const double rhs = bounds_to_rhs(lb, ub, inf);
    const RowBounds bounds = sense_rhs_to_bounds(highs_, sense, rhs);
    highs_ok(highs_.changeRowBounds(index, bounds.lower, bounds.upper), "changeRowBounds(sense)");
    //cerr << ">>>>> New sense for constraint " << index << ": " 
    //        << (sense == lp::Sense::GE ? "GE" : (sense == lp::Sense::LE ? "LE" : (sense == lp::Sense::EQ ? "EQ" : "UNKNOWN"))) 
    //        << endl;
}

void HiGHSSolverInterface::set_variable_lower_bound(int index, double bound) {
    const double ub = highs_.getLp().col_upper_[index];
    highs_ok(highs_.changeColBounds(index, bound, ub), "changeColBounds(lb)"); 
    //cerr << ">>>>> New lower bound for variable " << index << ": " << bound << endl;
}

void HiGHSSolverInterface::set_variable_upper_bound(int index, double bound) {
    const double lb = highs_.getLp().col_lower_[index];
    highs_ok(highs_.changeColBounds(index, lb, bound), "changeColBounds(ub)");
    //cerr << ">>>>> New upper bound for variable " << index << ": " << bound << endl;
}

void HiGHSSolverInterface::set_mip_gap(double gap) {
    highs_ok(highs_.setOptionValue("mip_rel_gap", gap), "setOptionValue(mip_rel_gap)");
}

void HiGHSSolverInterface::solve() {
    highs_ok(highs_.run(), "run");
}

void HiGHSSolverInterface::write_lp(const std::string &filename) const {
    highs_ok(highs_.writeModel(filename), "writeModel");
}

void HiGHSSolverInterface::print_failure_analysis() const {
    cout << highs_.modelStatusToString(highs_.getModelStatus()) << endl;
}

bool HiGHSSolverInterface::is_infeasible() const {
    //cerr << ">>>>> Is infeasible?  " << (highs_.getModelStatus() == HighsModelStatus::kInfeasible) << endl;
    return highs_.getModelStatus() == HighsModelStatus::kInfeasible;
}

bool HiGHSSolverInterface::is_unbounded() const {
    //cerr << ">>>>> Is unbounded?  " << (highs_.getModelStatus() == HighsModelStatus::kUnbounded) << endl;
    return highs_.getModelStatus() == HighsModelStatus::kUnbounded;
}

bool HiGHSSolverInterface::has_optimal_solution() const {
    //cerr << ">>>>> Has optimal solution?  " << (highs_.getModelStatus() == HighsModelStatus::kOptimal) << endl;
    return highs_.getModelStatus() == HighsModelStatus::kOptimal;
}

double HiGHSSolverInterface::get_objective_value() const {
    assert(has_optimal_solution());
    const HighsInfo& info = highs_.getInfo();
    //cerr << ">>>>> Objective value: " << info.objective_function_value << endl;
    return info.objective_function_value;
}

std::vector<double> HiGHSSolverInterface::extract_solution() const {
    assert (has_optimal_solution());
    const HighsSolution sol = highs_.getSolution();
    const int n = highs_.getNumCol();
    std::vector<double> x(n, 0.0);
    for (int i = 0; i < n; i++){
        x[i] = sol.col_value[i];
    }
    //cerr << ">>>>> Extracted solution: [";
    //for (int i = 0; i < n; ++i) {
    //    cerr << x[i];
    //    if (i + 1 < n) {
    //        cerr << ", ";
    //    }
    //}
    //cerr << "]" << endl;
    return x;
}

int HiGHSSolverInterface::get_num_variables() const {
    //cerr << ">>>>> Number of variables: " << highs_.getNumCol() << endl;
    return highs_.getNumCol();
}

int HiGHSSolverInterface::get_num_constraints() const {
    //cerr << ">>>>> Number of constraints: " << highs_.getNumRow() << endl;
    return highs_.getNumRow();
}

bool HiGHSSolverInterface::has_temporary_constraints() const {
    //cerr << ">>>>> Has temporary constraints?  " << (num_temporary_constraints > 0) << endl;
    return num_temporary_constraints != 0;
}

void HiGHSSolverInterface::print_statistics() const {
    cout << highs_.modelStatusToString(highs_.getModelStatus()) << endl;
} // namespace lp

}
