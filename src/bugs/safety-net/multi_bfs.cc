#include "globals.h"
#include "multi_bfs.h"
#include "operator.h"
#include "state.h"
#include "successor_generator.h"

#include <cassert>
#include <iostream>
#include <fstream>
using namespace std;

MultiBestFirstSearch::MultiBestFirstSearch() {
  expanded_states = 0;
  total_open_count = 0;
}

void MultiBestFirstSearch::add_heuristic(HeuristicFunc f, string name) {
  heuristics.push_back(f);
  heuristic_names.push_back(name);
  current_bucket.push_back(QUITE_A_LOT);
  best_heuristic.push_back(QUITE_A_LOT);
  open_lists.push_back(OpenList(100));
  open_count.push_back(0);
}

int MultiBestFirstSearch::search() {
  assert(heuristics.size() >= 1);
  cout << "Conducting multiple heuristic best first search." << endl;
  cout << "Using the following heuristics:" << endl;
  for(int i = 0; i < heuristics.size(); i++)
    cout << "(" << (i + 1) << ") " << heuristic_names[i] << endl;

  evaluate_state(*g_initial_state, *g_initial_state, 0);

  bool solution_found = false;
  while(total_open_count)
    for(int i = 0; i < heuristics.size(); i++)
      if(open_count[i]) {
	State current_state = pick_next_state(i, solution_found);
	if(solution_found) {
	  cout << "Solution found!" << endl;
	  extract_solution(current_state);
	  return expanded_states;
	}
	expand_state(current_state);
      }
  
  cout << "Completely explored state space -- no solution!" << endl;
  return expanded_states;
}

void MultiBestFirstSearch::evaluate_state(const State &state, const State &pred,
					  const Operator *op) {
  bool added = false;
  for(int i = 0; i < heuristics.size(); i++) {
    int h = (*(heuristics[i]))(state);
    // cout << heuristic_names[i] << ": " << h << endl;
    if(h != -1) {
      added = true;
      if(h >= open_lists[i].size()) {
	int new_size = open_lists[i].size() + 100;
	open_lists[i].resize(max(h + 1, new_size));
      }
      open_lists[i][h].push_back(state);
      open_count[i]++;
      total_open_count++;
      if(h < current_bucket[i])
	current_bucket[i] = h;
    }
  }
  if(added)
    predecessors.insert(make_pair(state, make_pair(pred, op)));
}

State MultiBestFirstSearch::pick_next_state(int heur_index, bool &solution_found) {
  OpenList &open = open_lists[heur_index];
  int &bucket = current_bucket[heur_index];
  
  while(open[bucket].empty())
    bucket++;

  State state = open[bucket].front();
  open[bucket].pop_front();
  total_open_count--;

  if(bucket < best_heuristic[heur_index]) {
    best_heuristic[heur_index] = bucket;
    cout << "Best heuristic value (" << heuristic_names[heur_index] << "): "
	 << best_heuristic[heur_index]
	 << " [expanded " << expanded_states << " state(s)]" << endl;
    if(bucket == 0)
      solution_found = true;
  }

  return state;
}

void MultiBestFirstSearch::expand_state(const State &state) {
  vector<const Operator *> applicable_ops;
  g_successor_generator->generate_applicable_ops(state, applicable_ops);
  for(int i = 0; i < applicable_ops.size(); i++) {
    State new_state(state, *applicable_ops[i]);
    if(!predecessors.count(new_state))
      evaluate_state(new_state, state, applicable_ops[i]);
  }

  expanded_states++;
}

void MultiBestFirstSearch::extract_solution(const State &goal_state) {
  vector<const Operator *> inverse_solution;
  State current_state = goal_state;
  for(;;) {
    // Using find() because array notation [] needs default constructor
    // for state.
    pair<State, const Operator *> p = predecessors.find(current_state)->second;
    if(p.second == 0)
      break;
    current_state = p.first;
    inverse_solution.push_back(p.second);
  }

  ofstream outfile("sas_plan");
  for(int i = inverse_solution.size() - 1; i >= 0; i--) {
    cout << inverse_solution[i]->get_name() << endl;
    outfile << "(" << inverse_solution[i]->get_name() << ")" << endl;
  }
}
