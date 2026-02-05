#include "highs_solver_interface.h"

#include <cassert>
#include <iostream>
#include <stdexcept>

using namespace std;

namespace lp {

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
    named_vector::NamedVector<LPVariable> vars = lp.get_variables();
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

        highs_ok(highs_.addRow(c.get_lower_bound(),
                               c.get_upper_bound(),
                               (int)idx.size(),
                               idx.data(),
                               val.data()),
                 "addRow");
    }
    num_permanent_constraints = lp.get_constraints().size();
    num_temporary_constraints = 0;

    //cout << "so far so good" << endl;
}

void HiGHSSolverInterface::add_temporary_constraints(
    const named_vector::NamedVector<LPConstraint> &constraints) {
    num_temporary_constraints += constraints.size();

    for (int i = 0; i < (int)constraints.size(); ++i) {
        const auto &c = constraints[i];
        const auto &idx = c.get_variables();
        const auto &val = c.get_coefficients();

        highs_ok(highs_.addRow(c.get_lower_bound(),
                               c.get_upper_bound(),
                               (int)idx.size(),
                               idx.data(),
                               val.data()),
                 "addRow(temporary_constraints)");
    }
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
}

void HiGHSSolverInterface::set_objective_coefficients(
    const std::vector<double>& coefficients)
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
}

void HiGHSSolverInterface::set_objective_coefficient(int index, double coefficient) {
    highs_ok(highs_.changeColCost(index, coefficient), "changeColCost");
}

void HiGHSSolverInterface::set_constraint_lower_bound(int index, double bound) {
    const double ub = highs_.getLp().row_upper_[index];
    highs_ok(highs_.changeRowBounds(index, bound, ub), "changeRowBounds(lb)");
}

void HiGHSSolverInterface::set_constraint_upper_bound(int index, double bound) {
    const double lb = highs_.getLp().row_lower_[index];
    highs_ok(highs_.changeRowBounds(index, lb, bound), "changeRowBounds(ub)");
}

void HiGHSSolverInterface::set_variable_lower_bound(int index, double bound) {
    const double ub = highs_.getLp().col_upper_[index];
    highs_ok(highs_.changeColBounds(index, bound, ub), "changeColBounds(lb)"); 
}

void HiGHSSolverInterface::set_variable_upper_bound(int index, double bound) {
    const double lb = highs_.getLp().col_lower_[index];
    highs_ok(highs_.changeColBounds(index, lb, bound), "changeColBounds(ub)");
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
    return highs_.getModelStatus() == HighsModelStatus::kInfeasible;
}

bool HiGHSSolverInterface::is_unbounded() const {
    return highs_.getModelStatus() == HighsModelStatus::kUnbounded;
}

bool HiGHSSolverInterface::has_optimal_solution() const {
    return highs_.getModelStatus() == HighsModelStatus::kOptimal;
}

double HiGHSSolverInterface::get_objective_value() const {
    assert(has_optimal_solution());
    const HighsInfo& info = highs_.getInfo();
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
    return x;
}

int HiGHSSolverInterface::get_num_variables() const {
    return highs_.getNumCol();
}

int HiGHSSolverInterface::get_num_constraints() const {
    return highs_.getNumRow();
}

bool HiGHSSolverInterface::has_temporary_constraints() const {
    return num_temporary_constraints != 0;
}

void HiGHSSolverInterface::print_statistics() const {
    cout << highs_.modelStatusToString(highs_.getModelStatus()) << endl;
} // namespace lp

}
