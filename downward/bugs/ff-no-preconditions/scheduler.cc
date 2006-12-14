#include "scheduler.h"

#include <cassert>
#include <climits>
#include <ctime>
#include <vector>
using namespace std;

Scheduler::Scheduler() {
  solution_found = false;
  expanded = -1;
  generated = -1;
}

void Scheduler::add_search_engine(SearchEngine *engine) {
  searchers.push_back(engine);
}

void Scheduler::add_time_limit(int seconds) {
  int clocks = seconds * CLOCKS_PER_SEC;
  assert(seconds <= 1 || (clocks > seconds && clocks > CLOCKS_PER_SEC));
  // Some rudimentary overflow checking.
  time_limits.push_back(clocks);
}

// TODO: This does not terminate gracefully for unsolvable tasks.
void Scheduler::search() {
  vector<bool> active(searchers.size(), true);
  for(int i = 0; i < time_limits.size(); i++) {
    for(int j = 0; j < searchers.size(); j++) {
      if(active[j]) {
	if(i == 0)
	  searchers[j]->initialize();
	int end_clock = clock() + time_limits[i];
	if(end_clock < clock()) // OVERFLOW?
	  end_clock = INT_MAX;
	while(clock() < end_clock) {
	  int outcome = searchers[j]->step();
	  if(outcome == SearchEngine::FAILED) {
	    active[j] = false;
	    delete searchers[j];
	    searchers[j] = 0;
	    break;
	  } else if(outcome == SearchEngine::SOLVED) {
	    solution_found = true;
	    plan = searchers[j]->get_plan();
	    expanded = searchers[j]->get_expanded_states();
	    generated = searchers[j]->get_generated_states();
	    return;
	  }
	}
	cout << endl << "Task switch..." << endl;
      }
    }
  }
}

bool Scheduler::found_solution() const {
  return solution_found;
}

const vector<const Operator *> Scheduler::get_plan() {
  return plan;
}

int Scheduler::get_expanded_states() const {
  return expanded;
}

int Scheduler::get_generated_states() const {
  return generated;
}
