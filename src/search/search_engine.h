#ifndef SEARCH_ENGINE_H
#define SEARCH_ENGINE_H

#include "operator_cost.h"
#include "operator_id.h"
#include "search_progress.h"
#include "search_space.h"
#include "search_statistics.h"
#include "state_registry.h"
#include "task_proxy.h"

#include <vector>

class Heuristic;

namespace options {
class OptionParser;
class Options;
}

namespace ordered_set {
template<typename T>
class OrderedSet;
}

enum SearchStatus {IN_PROGRESS, TIMEOUT, FAILED, SOLVED};

class SearchEngine {
public:
    using Plan = std::vector<OperatorID>;
private:
    SearchStatus status;
    bool solution_found;
    Plan plan;
protected:
    // Hold a reference to the task implementation and pass it to objects that need it.
    const std::shared_ptr<AbstractTask> task;
    // Use task_proxy to access task information.
    TaskProxy task_proxy;

    StateRegistry state_registry;
    SearchSpace search_space;
    SearchProgress search_progress;
    SearchStatistics statistics;
    int bound;
    OperatorCost cost_type;
    double max_time;

    virtual void initialize() {}
    virtual SearchStatus step() = 0;

    void set_plan(const Plan &plan);
    bool check_goal_and_set_plan(const GlobalState &state);
    int get_adjusted_cost(const OperatorProxy &op) const;
public:
    SearchEngine(const options::Options &opts);
    virtual ~SearchEngine();
    virtual void print_statistics() const;
    virtual void save_plan_if_necessary() const;
    bool found_solution() const;
    SearchStatus get_status() const;
    const Plan &get_plan() const;
    void search();
    const SearchStatistics &get_statistics() const {return statistics; }
    void set_bound(int b) {bound = b; }
    int get_bound() {return bound; }

    /* The following three methods should become functions as they
       do not require access to private/protected class members. */
    static void add_pruning_option(options::OptionParser &parser);
    static void add_options_to_parser(options::OptionParser &parser);
    static void add_succ_order_options(options::OptionParser &parser);
};

/*
  Print heuristic values of all heuristics evaluated in the evaluation context.
*/
extern void print_initial_h_values(const EvaluationContext &eval_context);

extern ordered_set::OrderedSet<OperatorID> collect_preferred_operators(
    EvaluationContext &eval_context,
    const std::vector<Heuristic *> &preferred_operator_heuristics);

#endif
