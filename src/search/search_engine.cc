#include <cassert>
#include <iostream>
#include <limits>
using namespace std;

#include "globals.h"
#include "search_engine.h"
#include "timer.h"
#include "option_parser.h"

SearchEngine::SearchEngine(const SearchEngineOptions &options)
    : search_space(static_cast<OperatorCost>(options.cost_type)) {
    solved = false;
    if (options.bound < 0) {
        cerr << "error: negative cost bound " << options.bound << endl;
        exit(2);
    }
    bound = options.bound;

    if (options.cost_type < 0 || options.cost_type >= MAX_OPERATOR_COST) {
        cerr << "error: unknown operator cost type: " << options.cost_type << endl;
        exit(2);
    }

    cost_type = static_cast<OperatorCost>(options.cost_type);
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


SearchEngineOptions::SearchEngineOptions()
    : cost_type(NORMAL),
      bound(numeric_limits<int>::max()) {
}

SearchEngineOptions::~SearchEngineOptions() {
}

void SearchEngineOptions::add_options_to_parser(
    NamedOptionParser &option_parser) {
    option_parser.add_int_option("cost_type",
                                 &cost_type,
                                 "operator cost adjustment type");
    option_parser.add_int_option("bound",
                                 &bound,
                                 "bound on plan cost", true);
}
