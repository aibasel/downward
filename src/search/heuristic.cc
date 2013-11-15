#include "heuristic.h"
#include "operator.h"
#include "option_parser.h"
#include "operator_cost.h"

#include <cassert>
#include <cstdlib>
#include <limits>

using namespace std;

Heuristic::Heuristic(const Options &opts)
    : cost_type(OperatorCost(opts.get_enum("cost_type"))) {
    heuristic = NOT_INITIALIZED;

    is_unit_cost = true;
    for (size_t i = 0; i < g_operators.size(); ++i) {
        if (get_adjusted_cost(g_operators[i]) != 1) {
            is_unit_cost = false;
            break;
        }
    }
}

Heuristic::~Heuristic() {
}

void Heuristic::set_preferred(const Operator *op) {
    if (!op->is_marked()) {
        op->mark();
        preferred_operators.push_back(op);
    }
}

void Heuristic::evaluate(const State &state) {
    if (heuristic == NOT_INITIALIZED)
        initialize();
    preferred_operators.clear();
    heuristic = compute_heuristic(state);
    for (int i = 0; i < preferred_operators.size(); i++)
        preferred_operators[i]->unmark();
    assert(heuristic == DEAD_END || heuristic >= 0);

    if (heuristic == DEAD_END) {
        // It is ok to have preferred operators in dead-end states.
        // This allows a heuristic to mark preferred operators on-the-fly,
        // selecting the first ones before it is clear that all goals
        // can be reached.
        preferred_operators.clear();
    }

#ifndef NDEBUG
    if (heuristic != DEAD_END) {
        for (int i = 0; i < preferred_operators.size(); i++)
            assert(preferred_operators[i]->is_applicable(state));
    }
#endif
    evaluator_value = heuristic;
}

bool Heuristic::is_dead_end() const {
    return evaluator_value == DEAD_END;
}

int Heuristic::get_heuristic() {
    // The -1 value for dead ends is an implementation detail which is
    // not supposed to leak. Thus, calling this for dead ends is an
    // error. Call "is_dead_end()" first.

    /*
      TODO: I've commented the assertion out for now because there is
      currently code that calls get_heuristic for dead ends. For
      example, if we use alternation with h^FF and h^cea and have an
      instance where the initial state has infinite h^cea value, we
      should expand this state since h^cea is unreliable. The search
      progress class will then want to print the h^cea value of the
      initial state since this is the "best know h^cea state" so far.

      However, we should clean up the code again so that the assertion
      is valid or rethink the interface so that we don't need it.
     */

    // assert(heuristic >= 0);
    if (heuristic == DEAD_END)
        return numeric_limits<int>::max();
    return heuristic;
}

void Heuristic::get_preferred_operators(std::vector<const Operator *> &result) {
    assert(heuristic >= 0);
    result.insert(result.end(),
                  preferred_operators.begin(),
                  preferred_operators.end());
}

bool Heuristic::reach_state(const State & /*parent_state*/,
                            const Operator & /*op*/, const State & /*state*/) {
    return false;
}

int Heuristic::get_value() const {
    return evaluator_value;
}

void Heuristic::evaluate(int, bool) {
    return;
    // if this is called, evaluate(const State &state) or set_evaluator_value(int val)
    // should already have been called
}

bool Heuristic::dead_end_is_reliable() const {
    return dead_ends_are_reliable();
}

void Heuristic::set_evaluator_value(int val) {
    evaluator_value = val;
}

int Heuristic::get_adjusted_cost(const Operator &op) const {
    return get_adjusted_action_cost(op, cost_type);
}

void Heuristic::add_options_to_parser(OptionParser &parser) {
    vector<string> cost_types;
    cost_types.push_back("NORMAL");
    cost_types.push_back("ONE");
    cost_types.push_back("PLUSONE");
    parser.add_enum_option("cost_type",
                           cost_types,
                           "NORMAL",
                           "operator cost adjustment type");
}

//this solution to get default values seems not optimal:
Options Heuristic::default_options() {
    Options opts = Options();
    opts.set<int>("cost_type", 0);
    return opts;
}
