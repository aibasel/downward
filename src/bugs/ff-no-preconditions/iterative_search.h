#ifndef ITERATIVE_SEARCH_H
#define ITERATIVE_SEARCH_H

#include <list>
#include <vector>
using namespace std;

#include "scheduler.h"
#include "state.h"

class SearchStatistics {
  bool may_undo_goal;
  int max_cost_limit;
  int max_layer;

  int closed_states_limit;
public:
  SearchStatistics();
  void start_goal(int goal_count);
  void finish_goal();
  void fail();
  void update_layer(bool may_undo_goal, int cost_limit, int layer);
  int generated_states;
  int expanded_states;
  int closed_states;

  void set_closed_limit(int limit);
  bool limit_exceeded() const;
};

class UniformCostSearcher {
  SearchStatistics *statistics; // shared by all searchers

  vector<pair<int, int> > goals;
  const State *initial_state;
  int new_goal_id;

  OpenList open;
  ClosedList closed;
  bool may_undo_goal;
  int cost_limit;
  int path_cost;
  State current_state;
  const State *predecessor;
  const Operator *current_operator;

  vector<const Operator *> actions;
public:
  enum {FAILED, SOLVED, IN_PROGRESS};

  UniformCostSearcher(SearchStatistics *statistics, const State *state,
		      const vector<bool> &solved_goals, int new_goal);
  int search_step();
  int get_new_goal() const {
    return new_goal_id;
  }
  const State &get_current_state() const {
    return current_state;
  }
  void extract_plan(vector<const Operator *> &result);
};

class IterativeSearchEngine : public SearchEngine {
  SearchStatistics statistics;
  bool solution_found;
  vector<const Operator *> plan;

  State current_state;
  int num_goals_solved;
  vector<bool> solved_goals;
  list<UniformCostSearcher> searchers;

  void initialize_searchers();
public:
  IterativeSearchEngine(int memory_limit);
  virtual ~IterativeSearchEngine();
  virtual void initialize();
  virtual int step();
  virtual bool found_solution() const;
  virtual const vector<const Operator *> &get_plan() const;
  virtual int get_expanded_states() const;
  virtual int get_generated_states() const;
};

#endif
