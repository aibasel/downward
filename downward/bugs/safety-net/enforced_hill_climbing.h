#ifndef ENFORCED_HILL_CLIMBING_H
#define ENFORCED_HILL_CLIMBING_H

#include <deque>
#include <map>
#include <vector>
#include <cassert>
using namespace std;

#include "best_first_search.h"
#include "state.h"

class Operator;

class EnforcedHillClimbingEngine {
  bool solution_found;
  int generated_states;
  int expanded_states;


  deque<State> open;
  ClosedList closed;
  vector<const Operator *> plan;

  int best_h;
  State best_state;
  bool climb(bool use_helpful_actions);
public:
  EnforcedHillClimbingEngine();
  void search();
  bool found_solution() const;
  const vector<const Operator *> &get_plan() const;
  int get_expanded_states() const;
  int get_generated_states() const;
};

#endif
