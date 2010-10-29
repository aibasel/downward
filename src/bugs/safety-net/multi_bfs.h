#ifndef MULTI_BFS_H
#define MULTI_BFS_H

#include "heuristic.h"
#include "state.h"

#include <deque>
#include <map>
#include <string>
#include <vector>
using namespace std;

class Operator;

class MultiBestFirstSearch {
  enum {QUITE_A_LOT = 1000000};

  typedef int (*HeuristicFunc)(const State &state);
  typedef vector<deque<State> > OpenList;

  vector<HeuristicFunc> heuristics;
  vector<string> heuristic_names;
  map<State, pair<State, const Operator *> > predecessors; // also used as Closed list
  vector<OpenList> open_lists;
  vector<int> open_count;
  vector<int> current_bucket;
  vector<int> best_heuristic;

  int expanded_states;
  int total_open_count;

  void evaluate_state(const State &state, const State &pred, const Operator *op);
  State pick_next_state(int heur_index, bool &solution_found);
  void expand_state(const State &state);
  void extract_solution(const State &goal_state);
public:
  MultiBestFirstSearch();
  void add_heuristic(HeuristicFunc f, string name);
  int search();
};

#endif
