#include "best_first_search.h"
#include "globals.h"
#include "operator.h"
#include "successor_generator.h"

#include <cassert>
using namespace std;

BestFirstSearchEngine::BestFirstSearchEngine() {
  solution_found = false;
  generated_states = 0;
  expanded_states = 0;
  best_h = 1000000;
}

bool BestFirstSearchEngine::found_solution() const {
  return solution_found;
}

const vector<const Operator *> &BestFirstSearchEngine::get_plan() const {
  assert(solution_found);
  return plan;
}

int BestFirstSearchEngine::get_expanded_states() const {
  return expanded_states;
}

int BestFirstSearchEngine::get_generated_states() const {
  return generated_states;
}

int BestFirstSearchEngine::check_h_value(const State &new_state, int new_h) {
  if(new_h < best_h) {
    best_h = new_h;
    cout << "Best heuristic value: " << best_h
	 << " [expanded " << expanded_states << " state(s)]" << endl;
    
    if(best_h == 0) {
      cout << "Solution found!" << endl;
      solution_found = true;
      closed.get_path_from_initial_state(new_state, plan);
    }
  }
  return best_h;
}

void BestFirstSearchEngine::expand_state(const State &current_state,
					 const Operator &op) {
  State new_state(current_state, op);
  if(!closed.contains(new_state)) {
    vector<const Operator *> new_helpful_actions;
    int new_h = new_state.heuristic_and_helpful_actions(new_helpful_actions);
    generated_states++;
    if(new_h != -1) {
      open1.insert(new_h, make_pair(new_state, new_helpful_actions));
      closed.insert(new_state, current_state, &op);
    }
  }
}

void BestFirstSearchEngine::search() {
  // (with helpful actions)
  // Tip from Jörg: When generating nodes with the same heuristic value,
  // expand them in first-in first-out order.

  cout << "Conducting best first search." << endl;
  vector<const Operator *> helpful_actions;
  int init_h = g_initial_state->heuristic_and_helpful_actions(helpful_actions);
  generated_states++;
  if(init_h != -1) {
    open1.insert(init_h, make_pair(*g_initial_state, helpful_actions));
    closed.insert(*g_initial_state, *g_initial_state, 0);
  }
  
  int open1_cost = 0, open2_cost = 0;
  while(!open1.empty() || !open2.empty()) {
    bool prefer_open1 = (1 * open1_cost <= 10 * open2_cost);
    if(prefer_open1 && !open1.empty() || open2.empty()) {
      expanded_states++;
      // Remove from first open queue and put into second open queue.
      int current_h = open1.min_key();
      pair<State, vector<const Operator *> > &entry = open1.min_value();
      State current_state = entry.first;
      vector<const Operator *> helpful_actions;
      helpful_actions.swap(entry.second);
      open1.remove_min();
      open2.insert(current_h, current_state);
      if(check_h_value(current_state, current_h) == 0)
	return;
      open1_cost += helpful_actions.size();
      for(int i = 0; i < helpful_actions.size(); i++) {
	assert(helpful_actions[i]->is_applicable(current_state));
	expand_state(current_state, *helpful_actions[i]);
      }
    } else {
      // Remove from second open queue.
      int current_h = open2.min_key();
      State current_state = open2.min_value();
      open2.remove_min();
      if(check_h_value(current_state, current_h) == 0)
	return;
      vector<const Operator *> applicable_ops;
      g_successor_generator->generate_applicable_ops(current_state, applicable_ops);
      for(int i = 0; i < applicable_ops.size(); i++)
	expand_state(current_state, *applicable_ops[i]);
      open2_cost += applicable_ops.size();
    }
  }
  cout << "Completely explored state space -- no solution!" << endl;
}

ClosedList::ClosedList() {
}

bool ClosedList::contains(const State &state) const {
  return closed.count(state) != 0;
}

void ClosedList::insert(const State &state, const State &predecessor,
			const Operator *reached_by) {
  closed.insert(make_pair(state, PredecessorInfo(predecessor, reached_by)));
}

void ClosedList::get_path_from_initial_state(const State &state,
					     vector<const Operator *> &path) const {
  vector<const Operator *> inverse_path;
  State current_state = state;
  for(;;) {
    const PredecessorInfo &info = closed.find(current_state)->second;
    if(info.reached_by == 0)
      break;
    inverse_path.push_back(info.reached_by);
    current_state = info.predecessor;
  }

  for(int i = inverse_path.size() - 1; i >= 0; i--)
    path.push_back(inverse_path[i]);
}

void ClosedList::clear() {
  closed.clear();
}
