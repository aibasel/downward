#ifndef BEST_FIRST_SEARCH_H
#define BEST_FIRST_SEARCH_H

#include <deque>
#include <map>
#include <vector>
using namespace std;

#include "state.h"
#include "scheduler.h"

class Operator;

class ClosedList {
  struct PredecessorInfo {
    PredecessorInfo(const State *pred, const Operator *reached)
      : predecessor(pred), reached_by(reached) {}
    const State *predecessor;
    const Operator *reached_by;
  };
  map<State, PredecessorInfo> closed;
public:
  ClosedList();
  bool contains(const State &state) const;
  const State *insert(const State &state,
		      const State *predecessor,
		      const Operator *reached_by);
  void get_path_from_initial_state(const State &state,
				   vector<const Operator *> &path) const;
  int size() const;
  void clear();
};

class OpenList {
  typedef pair<const State *, const Operator *> Entry;
  typedef deque<Entry> Bucket;
  vector<Bucket> buckets;
  int lowest_bucket;
  int size;
public:
  OpenList();
  void insert(int key, const State *parent, const Operator *operator_);
  int min();
  Entry remove_min();
  bool empty() const;
  void clear();
};

class BestFirstSearchEngine : public SearchEngine {
  int open1_expansions, open2_expansions;
  vector<const Operator *> helpful_actions, all_actions;
  State current_state;
  const State *predecessor;
  const Operator *current_operator;

  bool solution_found;
  int generated_states;
  int expanded_states;

  int best_h;

  OpenList open1, open2, open3, open4;
  ClosedList closed;
  vector<const Operator *> plan;

  int check_h_value(const State &new_state, int new_h);
  void expand_state(const State &current_state, const Operator &op);
public:
  BestFirstSearchEngine();
  ~BestFirstSearchEngine();
  virtual void initialize();
  virtual int step();
  virtual bool found_solution() const;
  virtual const vector<const Operator *> &get_plan() const;
  virtual int get_expanded_states() const;
  virtual int get_generated_states() const;
};

#endif
