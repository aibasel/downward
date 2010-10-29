#include "best_first_search.h"
#include "globals.h"
#include "operator.h"
#include "successor_generator.h"

#include <cassert>
using namespace std;

BestFirstSearchEngine::BestFirstSearchEngine()
  : current_state(*g_initial_state) {
  solution_found = false;
  generated_states = 0;
  expanded_states = 0;
  best_h = 1000000;
}

BestFirstSearchEngine::~BestFirstSearchEngine() {
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

void BestFirstSearchEngine::initialize() {
  cout << "Conducting best first search." << endl;

  regular_expansions = 0;
  preferred_expansions = 0;
  predecessor = 0;
  current_operator = 0;
}

int BestFirstSearchEngine::step() {
  // Tip from Jörg: When generating nodes with the same heuristic value,
  // expand them in first-in first-out order.

  // Invariants:
  // - current_state is the next state for which we want to compute the heuristic.
  // - predecessor is a permanent pointer to the predecessor of that state.
  // - current_operator is the operator which leads to current_state from predecessor.
  
  if(!closed.contains(current_state)) {
    
    // Compute heuristic and generate successors.
    const State *parent_ptr = closed.insert(current_state, predecessor,
					    current_operator);
    int h, ff_h;
    current_state.heuristic_and_helpful_actions(h, ff_h, helpful_actions);
    expanded_states++;
    
    if(ff_h != -1) {
      if(h != -1 && h < best_h) {
	best_h = h;
	if(g_multi_heuristic)
	  preferred_expansions -= 1000; // HACK! Helpful action benefit.
	if(best_h == 0) {
	  cout << "Solution found!" << endl;
	  solution_found = true;
	  closed.get_path_from_initial_state(current_state, plan);
	  return SOLVED;
	} else {
	  cout << "Best heuristic value: " << best_h
	       << " [expanded " << expanded_states << " state(s)]" << endl;
	}
      }  
      
      for(int i = 0; i < helpful_actions.size(); i++) {
	if(h != -1)
	  open1.insert(h, parent_ptr, helpful_actions[i]);
	if(g_multi_heuristic)
	  open3.insert(ff_h, parent_ptr, helpful_actions[i]);
      }
      generated_states += helpful_actions.size();
      helpful_actions.clear();
      
      g_successor_generator->generate_applicable_ops(current_state, all_actions);
      
      for(int i = 0; i < all_actions.size(); i++) {
	if(h != -1)
	  open2.insert(h, parent_ptr, all_actions[i]);
	if(g_multi_heuristic)
	  open4.insert(ff_h, parent_ptr, all_actions[i]);
      }
      generated_states += all_actions.size();
      all_actions.clear();
    }
  }

  // open1: helpful actions, CG heuristic
  // open2: all actions, CG heuristic
  // open3: helpful actions, FF heuristic
  // open4: all actions, FF heuristic

  if(g_multi_heuristic && open4.empty() ||
     !g_multi_heuristic && open2.empty()) {
    cout << "Completely explored state space -- no solution!" << endl;
    return FAILED;
  }
  
  // Fetch new state.
  static bool use_ff = true;
  use_ff = !use_ff && g_multi_heuristic;

  if(!use_ff && open1.empty() && open2.empty())
    use_ff = true;

  bool preferred = 1 * preferred_expansions <= 1 * regular_expansions;
  if(use_ff) {
    if(preferred) {
      if(open3.empty())
	preferred = false;
    } else {
      if(open4.empty())
	preferred = true;
    }
  } else {
    if(preferred) {
      if(open1.empty())
	preferred = false;
    } else {
      if(open2.empty())
	preferred = true;
    }
  }

  OpenList &open = preferred
    ? (use_ff ? open3 : open1)
    : (use_ff ? open4 : open2);
  if(preferred)
    preferred_expansions++;
  else
    regular_expansions++;

  /*
  cout << open1.empty() << open2.empty()
       << open3.empty() << open4.empty()
       << (&open == &open1) << (&open == &open2)
       << (&open == &open3) << (&open == &open4)
       << open.empty() << flush;
  */
  pair<const State *, const Operator *> next_pair = open.remove_min();
  predecessor = next_pair.first;
  current_operator = next_pair.second;

  current_state = State(*predecessor, *current_operator);

  return IN_PROGRESS;
}

// Implementation of Open List.

OpenList::OpenList() {
  lowest_bucket = 0;
  size = 0;
}

void OpenList::insert(int key, const State *parent, const Operator *operator_) {
  if(key >= buckets.size())
    buckets.resize(key + 1);
  else if(key < lowest_bucket)
    lowest_bucket = key;
  buckets[key].push_back(Entry(parent, operator_));
  size++;
}

int OpenList::min() {
  assert(size);
  while(buckets[lowest_bucket].empty())
    lowest_bucket++;
  return lowest_bucket;
}

OpenList::Entry OpenList::remove_min() {
  assert(size > 0);
  while(buckets[lowest_bucket].empty())
    lowest_bucket++;
  size--;
  Entry result = buckets[lowest_bucket].front();
  buckets[lowest_bucket].pop_front();
  return result;
}

bool OpenList::empty() const {
  return size == 0;
}

void OpenList::clear() {
  buckets.clear();
  lowest_bucket = 0;
  size = 0;
}

// Implementation of Closed List.

ClosedList::ClosedList() {
}

bool ClosedList::contains(const State &state) const {
  return closed.count(state) != 0;
}

const State *ClosedList::insert(const State &state, const State *predecessor,
				const Operator *reached_by) {
  map<State, PredecessorInfo>::iterator it =
    closed.insert(make_pair(state, PredecessorInfo(predecessor, reached_by))).first;
  return &it->first;
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
    current_state = *info.predecessor;
  }

  for(int i = inverse_path.size() - 1; i >= 0; i--)
    path.push_back(inverse_path[i]);
}

int ClosedList::size() const {
  return closed.size();
}

void ClosedList::clear() {
  closed.clear();
}
