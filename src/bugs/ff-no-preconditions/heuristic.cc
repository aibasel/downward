#include "cache.h"
#include "domain_transition_graph.h"
#include "ff_heuristic.h"
#include "globals.h"
#include "heuristic.h"
#include "operator.h"
#include "state.h"

#include <cassert>
#include <vector>
using namespace std;

Heuristic::Heuristic(const State &s) : state(s) {
}

int Heuristic::get_heuristic(int goal_max) {
  return cg(goal_max);
}

int min(int a, int b) {return a < b ? a : b;}

int Heuristic::cg(int goal_max) {
#if 0
  int result = 0;
  const int COUNT = 1;
  for(int i = 0; i < min(COUNT, g_goal.size() / 2); i++) {
    int var_no = g_goal[i].first, from = state[var_no], to = g_goal[i].second;
    if(from != to)
      result++;
  }
  for(int i = 0; i < min(COUNT, g_goal.size() / 2); i++) {
    int var_no = g_goal[g_goal.size() - i - 1].first;
    int from = state[var_no];
    int to = g_goal[g_goal.size() - i - 1].second;
    if(from != to)
      result++;
  }
  return result;
#endif

  cg_setup_domain_transition_graphs();
  int heuristic = 0;
  for(int i = 0; i < min(g_goal.size(), goal_max); i++) {
    int var_no = g_goal[i].first, from = state[var_no], to = g_goal[i].second;
    DomainTransitionGraph *dtg = g_transition_graphs[var_no];
    heuristic += get_transition_cost(dtg, from, to);
    if(heuristic >= QUITE_A_LOT)
      return -1;
  }
  return heuristic;
}

void Heuristic::cg_setup_domain_transition_graphs() {
  for(int var = 0; var < g_transition_graphs.size(); var++) {
    DomainTransitionGraph *dtg = g_transition_graphs[var];
    for(int i = 0; i < dtg->nodes.size(); i++) {
      dtg->nodes[i].distances.clear();
      dtg->nodes[i].helpful_transitions.clear();
    }
  }
}

static int s_helpful_action_extraction_counter = 0;

/*
A note on caching:

It seems wasteful to do so little caching.
Why store costs from "start_val" to "goal_val" only,
when values have been computed from "start_val" to any value?

Check if this can be done better without causing problems with helpful actions
et cetera.
 */

#define USE_CACHE true

int Heuristic::get_transition_cost(DomainTransitionGraph *dtg, int start_val,
				   int goal_val) {
  if(start_val == goal_val)
    return 0;

  int var_no = dtg->var;

  // Check cache.
  if(USE_CACHE && g_cache->is_cached(var_no)) {
    int cached_val = g_cache->lookup(var_no, state, start_val, goal_val);
    if(cached_val != Cache::NOT_COMPUTED) {
      g_cache_hits++;
      return cached_val;
    }
  } else {
    g_cache_misses++;
  }

  ValueNode *start = &dtg->nodes[start_val];
  if(start->distances.empty()) {
    int base_transition_cost = dtg->is_axiom ? 0 : 1;

    // Initialize data of initial node.
    start->distances.resize(dtg->nodes.size(), QUITE_A_LOT);
    start->helpful_transitions.resize(dtg->nodes.size(), 0);
    start->distances[start_val] = 0;
    start->reached_from = 0;
    start->reached_by = 0;
    start->children_state.resize(dtg->local_to_global_child.size());
    for(int i = 0; i < dtg->local_to_global_child.size(); i++)
      start->children_state[i] = state[dtg->local_to_global_child[i]];

    // Initialize Heap for Dijkstra's algorithm.
    vector<vector<ValueNode *> > buckets;
    buckets.resize(10); // Arbitrary initial size, expanded as needed.
    int bucket_contents = 1;
    buckets[0].push_back(start);
    int source_distance = 0;

    // Dijkstra algorithm main loop.
    while(bucket_contents) {
      for(int pos = 0; pos < buckets[source_distance].size(); pos++) {
	ValueNode *source = buckets[source_distance][pos];

	assert(start->distances[source->value] <= source_distance);
	if(start->distances[source->value] < source_distance)
	  continue;

	ValueTransitionLabel *current_helpful_transition =
	  start->helpful_transitions[source->value];
	
	// Set children state for all nodes but the initial.
	if(source_distance) {
	  source->children_state = source->reached_from->children_state;
	  vector<PrevailCondition> &prevail = source->reached_by->prevail;
	  for(int k = 0; k < prevail.size(); k++)
	    source->children_state[prevail[k].local_var] = prevail[k].value;
	}

	// Scan outgoing transitions.
	for(int i = 0; i < source->transitions.size(); i++) {
	  ValueTransition *transition = &source->transitions[i];
	  ValueNode *target = transition->target;
	  int *target_distance_ptr = &start->distances[target->value];

	  if(*target_distance_ptr > source_distance + base_transition_cost) {
	    // Scan labels of the transition.
	    for(int j = 0; j < transition->labels.size(); j++) {
	      ValueTransitionLabel *label = &transition->labels[j];
	      int new_distance = source_distance + base_transition_cost;
	      vector<PrevailCondition> &prevail = label->prevail;
	      for(int k = 0; k < prevail.size(); k++) {
		int local_var = prevail[k].local_var;
		int current_val = source->children_state[local_var];
		new_distance += get_transition_cost(prevail[k].prev_dtg,
						    current_val,
						    prevail[k].value);
	      }

	      if(new_distance == 0)
		new_distance = 1;  // HACK for axioms

	      if(*target_distance_ptr > new_distance) {
		// Update node in heap and update its internal state.
		*target_distance_ptr = new_distance;
		target->reached_from = source;
		target->reached_by = label;

		if(current_helpful_transition == 0) {
		  // This transition starts at the start node;
		  // no helpful transitions recorded yet.
		  start->helpful_transitions[target->value] = label;
		} else {
		  start->helpful_transitions[target->value] = current_helpful_transition;
		}

		if(new_distance >= buckets.size())
		  buckets.resize(max(new_distance + 1,
				     static_cast<int>(buckets.size()) * 2));
		buckets[new_distance].push_back(target);
		bucket_contents++;
	      }
	    }
	  }
	}
      }
      bucket_contents -= buckets[source_distance].size();
      buckets[source_distance].clear();
      source_distance++;
    }
  }

  int transition_cost = start->distances[goal_val];

  // Fill cache.
  if(USE_CACHE && g_cache->is_cached(var_no)) {
    g_cache->store(var_no, state, start_val, goal_val, transition_cost);
    if(g_downward_helpful_actions)
      g_cache->store_helpful_action(var_no, state, start_val, goal_val,
				    start->helpful_transitions[goal_val]);
  }

  return transition_cost;
}

void Heuristic::get_helpful_actions(vector<const Operator *> &result, int goal_max) {
  assert(result.empty());
  if(g_downward_helpful_actions)
    get_helpful_actions_cg(result, goal_max);

  if(g_ff_helpful_actions || g_mixed_helpful_actions && result.empty()) {
    if(!g_multi_heuristic)
      g_ff_heuristic->set_state(&state);
    g_ff_heuristic->get_helpful_actions(result);
  }
}

void Heuristic::get_helpful_actions_cg(vector<const Operator *> &result, int goal_max) {
  s_helpful_action_extraction_counter++; // Clear "dirty bits"
  for(int i = 0; i < min(g_goal.size(), goal_max); i++) {
    int var_no = g_goal[i].first, to = g_goal[i].second;
    DomainTransitionGraph *dtg = g_transition_graphs[var_no];
    extract_helpful_actions_cg(dtg, to, result);
  }
}

void Heuristic::extract_helpful_actions_cg(DomainTransitionGraph *dtg, int to,
					   vector<const Operator *> &result) {
  int var_no = dtg->var;
  int from = state[var_no];
  if(from == to)
    return;

  // Avoid checking the same layer twice via different paths of recursion.
  if(dtg->last_helpful_action_extraction_time == s_helpful_action_extraction_counter)
    return;
  dtg->last_helpful_action_extraction_time = s_helpful_action_extraction_counter;

  ValueTransitionLabel *helpful = 0;
  int cost;
  // Check cache.
  if(USE_CACHE && g_cache->is_cached(var_no))
    helpful = g_cache->lookup_helpful_action(var_no, state, from, to);
  if(helpful) {
    cost = g_cache->lookup(var_no, state, from, to);
  } else {
    ValueNode *start_node = &dtg->nodes[from];
    assert(!start_node->helpful_transitions.empty());
    helpful = start_node->helpful_transitions[to];
    cost = start_node->distances[to];
  }

  if(cost == 1 && !helpful->op->is_axiom() && helpful->op->is_applicable(state)) {
    // Transition immediately applicable, all prevail conditions already achieved.
    result.push_back(helpful->op);
  } else {
    // Recursively compute helpful actions for the prevail condition variables.
    const vector<PrevailCondition> &prevail = helpful->prevail;
    for(int i = 0; i < prevail.size(); i++)
      extract_helpful_actions_cg(prevail[i].prev_dtg,
				 prevail[i].value, result);
  }
}
