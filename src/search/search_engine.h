#ifndef SEARCH_ENGINE_H
#define SEARCH_ENGINE_H

#include <vector>

class Heuristic;
class OptionParser;
class Options;

#include "global_operator.h"
#include "operator_cost.h"
#include "search_progress.h"
#include "search_space.h"
#include "search_statistics.h"

enum SearchStatus {IN_PROGRESS, TIMEOUT, FAILED, SOLVED};

class SearchEngine {
public:
    typedef std::vector<const GlobalOperator *> Plan;
private:
    SearchStatus status;
    bool solution_found;
    Plan plan;
protected:
    SearchSpace search_space;
    SearchProgress search_progress;
    /*
      TODO: It would perhaps be nice to rename the following attribute
      to "statistics", but then we'd also need to rename the method
      currently called "statistics" (which would also be a good idea
      because we generally use verbs for method names).
    */
    SearchStatistics search_statistics;
    int bound;
    OperatorCost cost_type;
    double max_time;

    virtual void initialize() {}
    virtual SearchStatus step() = 0;

    void set_plan(const Plan &plan);
    bool check_goal_and_set_plan(const GlobalState &state);
    int get_adjusted_cost(const GlobalOperator &op) const;
public:
    SearchEngine(const Options &opts);
    virtual ~SearchEngine();
    virtual void statistics() const;
    virtual void heuristic_statistics() const {}
    virtual void save_plan_if_necessary() const;
    bool found_solution() const;
    SearchStatus get_status() const;
    const Plan &get_plan() const;
    void search();
    SearchProgress get_search_progress() const {return search_progress; }
    void set_bound(int b) {bound = b; }
    int get_bound() {return bound; }
    static void add_options_to_parser(OptionParser &parser);
};

#endif
