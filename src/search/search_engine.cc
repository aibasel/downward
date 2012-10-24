#include <cassert>
#include <iostream>
#include <limits>
using namespace std;

#include "globals.h"
#include "operator_cost.h"
#include "option_parser.h"
#include "search_engine.h"
#include "timer.h"

SearchEngine::SearchEngine(const Options &opts)
    : search_space(OperatorCost(opts.get_enum("cost_type"))),
      cost_type(OperatorCost(opts.get_enum("cost_type"))) {
    solved = false;
    if (opts.get<int>("bound") < 0) {
        cerr << "error: negative cost bound " << opts.get<int>("bound") << endl;
        exit(2);
    }
    bound = opts.get<int>("bound");
}

SearchEngine::~SearchEngine() {
}

void SearchEngine::statistics() const {
}

bool SearchEngine::found_solution() const {
    return solved;
}

const SearchEngine::Plan &SearchEngine::get_plan() const {
    assert(solved);
    return plan;
}

void SearchEngine::set_plan(const Plan &p) {
    solved = true;
    plan = p;
}

void SearchEngine::search() {
    initialize();
    Timer timer;
    while (step() == IN_PROGRESS)
        ;
    cout << "Actual search time: " << timer
         << " [t=" << g_timer << "]" << endl;
}

bool SearchEngine::check_goal_and_set_plan(const State &state) {
    if (test_goal(state)) {
        cout << "Solution found!" << endl;
        Plan plan;
        search_space.trace_path(state, plan);
        set_plan(plan);
        return true;
    }
    return false;
}

void SearchEngine::save_plan_if_necessary() const {
    if (found_solution())
        save_plan(get_plan(), 0);
}

int SearchEngine::get_adjusted_cost(const Operator &op) const {
    return get_adjusted_action_cost(op, cost_type);
}

void SearchEngine::add_options_to_parser(OptionParser &parser) {
    ::add_cost_type_option_to_parser(parser);
    parser.add_option<int>(
        "bound",
        "depth bound on g-values. Cutoffs are always performed according to "
        "the real cost, regardless of the cost_type parameter",  "infinity");
}
