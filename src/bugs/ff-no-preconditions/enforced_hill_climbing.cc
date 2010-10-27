#include "best_first_search.h" // ClosedList
#include "enforced_hill_climbing.h"
#include "globals.h"
#include "operator.h"
#include "successor_generator.h"

#include <cassert>
using namespace std;

EnforcedHillClimbingEngine::EnforcedHillClimbingEngine()
  : best_state(*g_initial_state) {
  solution_found = false;
  generated_states = 0;
  expanded_states = 0;
}

bool EnforcedHillClimbingEngine::found_solution() const {
  return solution_found;
}

const vector<const Operator *> &EnforcedHillClimbingEngine::get_plan() const {
  assert(solution_found);
  return plan;
}

int EnforcedHillClimbingEngine::get_expanded_states() const {
  return expanded_states;
}

int EnforcedHillClimbingEngine::get_generated_states() const {
  return generated_states;
}

bool EnforcedHillClimbingEngine::climb(bool use_helpful_actions) {
  open.clear();
  open.push_back(best_state);
  closed.clear();
  closed.insert(best_state, best_state, 0);
  while(!open.empty()) {
    State current_state = open.front();
    open.pop_front();

    int h;
    vector<const Operator *> operators;
    if(use_helpful_actions) {
      h = current_state.heuristic_and_helpful_actions(operators);
      g_successor_generator->generate_applicable_ops(current_state, operators); // HACK
    } else {
      h = current_state.heuristic();
      g_successor_generator->generate_applicable_ops(current_state, operators);
    }
    expanded_states++;
    generated_states++;

    if(h < best_h) {
      cout << "Best heuristic value: " << h
	   << " [expanded " << expanded_states << " state(s)]" << endl;
      best_h = h;
      best_state = current_state;
      closed.get_path_from_initial_state(best_state, plan);
      return true;
    } else if(h != -1) {
      for(int i = 0; i < operators.size(); i++) {
	const Operator *op = operators[i];
	State successor(current_state, *op);
	if(!closed.contains(successor)) {
	  closed.insert(successor, current_state, op);
	  open.push_back(successor);
	}
      }
    }
  }
  return false;
}

void EnforcedHillClimbingEngine::search() {
  // with helpful actions
  cout << "Conducting enforced hill climbing." << endl;
  best_h = 1000000;
  while(best_h != 0) {
    if(!climb(true) && !climb(false)) {
      cout << "Enforced Hill Climbing failed!" << endl;
      return;
    }
  }
  cout << "Solution found!" << endl;
  solution_found = true;
}
