#ifndef BEST_FIRST_SEARCH_H
#define BEST_FIRST_SEARCH_H

#include <deque>
#include <map>
#include <vector>
#include <cassert>
using namespace std;

#include "state.h"

class Operator;

class ClosedList {
  struct PredecessorInfo {
    PredecessorInfo(const State &pred, const Operator *reached)
      : predecessor(pred), reached_by(reached) {}
    State predecessor;
    const Operator *reached_by;
  };
  map<State, PredecessorInfo> closed;
public:
  ClosedList();
  bool contains(const State &state) const;
  void insert(const State &state, const State &predecessor,
	      const Operator *reached_by);
  void get_path_from_initial_state(const State &state,
				   vector<const Operator *> &path) const;
  void clear();
};

template<class Entry>
class OpenList {
  typedef deque<Entry> Bucket;
  vector<Bucket> buckets;
  int lowest_bucket;
  int size;

public:
  OpenList() {
    lowest_bucket = 0;
    size = 0;
  }
  void insert(int key, const Entry &entry) {
    if(key >= buckets.size())
      buckets.resize(key + 1);
    else if(key < lowest_bucket)
      lowest_bucket = key;
    buckets[key].push_back(entry);
    size++;
  }
  int min_key() {
    assert(size > 0);
    while(buckets[lowest_bucket].empty())
      lowest_bucket++;
    return lowest_bucket;
  }
  Entry &min_value() {
    assert(!buckets[lowest_bucket].empty()); // must call min_key() first
    return buckets[lowest_bucket].front();
  }
  void remove_min() {
    assert(!buckets[lowest_bucket].empty()); // must call min_key() first
    buckets[lowest_bucket].pop_front();
    size--;
  }
  bool empty() const {
    return size == 0;
  }
};

class BestFirstSearchEngine {
  bool solution_found;
  int generated_states;
  int expanded_states;

  int best_h;

  // map<int, deque<pair<State, vector<Operator *> > > > open1;
  // map<int, deque<State> > open2;

  OpenList<pair<State, vector<const Operator *> > > open1;
  OpenList<State> open2;
  ClosedList closed;
  vector<const Operator *> plan;

  int check_h_value(const State &new_state, int new_h);
  void expand_state(const State &current_state, const Operator &op);
public:
  BestFirstSearchEngine();
  void search();
  bool found_solution() const;
  const vector<const Operator *> &get_plan() const;
  int get_expanded_states() const;
  int get_generated_states() const;
};

#endif
