#ifndef CACHE_H
#define CACHE_H

#include <vector>
using namespace std;

class State;
class ValueTransitionLabel;

class Cache {
  vector<vector<int> > cache;
  vector<vector<ValueTransitionLabel *> > helpful_transition_cache;
  vector<vector<int> > depends_on;

  int get_index(int var, const State &state, int from_val, int to_val) const;
public:
  static const int NOT_COMPUTED = -2;

  Cache();
  bool is_cached(int var) const {
    return !cache[var].empty();
  };
  int lookup(int var, const State &state, int from_val, int to_val) const {
    return cache[var][get_index(var, state, from_val, to_val)];
  }
  void store(int var, const State &state, int from_val, int to_val, int cost) {
    cache[var][get_index(var, state, from_val, to_val)] = cost;
  }
  ValueTransitionLabel *lookup_helpful_transition(int var, const State &state,
						  int from_val, int to_val) const {
    return helpful_transition_cache[var][get_index(var, state, from_val, to_val)];
  }
  void store_helpful_transition(int var, const State &state, int from_val, int to_val,
			    ValueTransitionLabel *helpful_transition) {
    helpful_transition_cache[var][get_index(var, state, from_val, to_val)] = helpful_transition;
  }
};

#endif
