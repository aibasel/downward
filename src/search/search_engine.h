#ifndef SEARCH_ENGINE_H
#define SEARCH_ENGINE_H

#include <vector>

class Heuristic;
class OptionParser;
class Options;

#include "operator.h"
#include "search_space.h"
#include "search_progress.h"
#include "operator_cost.h"

class SearchEngine {
public:
    typedef std::vector<const Operator *> Plan;
private:
    bool solved;
    Plan plan;
protected:
    SearchSpace search_space;
    SearchProgress search_progress;
    int bound;
    OperatorCost cost_type;

    enum {FAILED, SOLVED, IN_PROGRESS};
    virtual void initialize() {}
    virtual int step() = 0;

    void set_plan(const Plan &plan);
    bool check_goal_and_set_plan(const State &state);
    int get_adjusted_cost(const Operator &op) const;
public:
    SearchEngine(const Options &opts);
    virtual ~SearchEngine();
    virtual void statistics() const;
    virtual void heuristic_statistics() const {}
    virtual void save_plan_if_necessary() const;
    bool found_solution() const;
    const Plan &get_plan() const;
    void search();
    SearchProgress get_search_progress() const {return search_progress; }
    void set_bound(int b) {bound = b; }
    int get_bound() {return bound; }
    static void add_options_to_parser(OptionParser &parser);
};

#endif
