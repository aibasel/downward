#ifndef SCHEDULER_H
#define SCHEDULER_H

#include <deque>
#include <map>
#include <vector>
using namespace std;

#include "state.h"

class SearchEngine {
public:
  enum {FAILED, SOLVED, IN_PROGRESS};
  virtual ~SearchEngine() {}
  virtual void initialize() {}
  virtual int step() = 0;
  virtual bool found_solution() const = 0;
  virtual const vector<const Operator *> &get_plan() const = 0;
  virtual int get_expanded_states() const = 0;
  virtual int get_generated_states() const = 0;
};

class Scheduler {
  vector<SearchEngine *> searchers;
  vector<int> time_limits;

  bool solution_found;
  vector<const Operator *> plan;
  int expanded;
  int generated;
public:
  Scheduler();
  void add_search_engine(SearchEngine *engine);
  void add_time_limit(int seconds);
  void search();
  bool found_solution() const;
  const vector<const Operator *> get_plan();
  int get_expanded_states() const;
  int get_generated_states() const;
};

#endif
