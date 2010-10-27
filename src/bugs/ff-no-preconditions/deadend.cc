#include "causal_graph.h"
#include "deadend.h"
#include "domain_transition_graph.h"
#include "globals.h"
#include "horn.h"
#include "operator.h"
#include "state.h"

#include <cassert>
#include <iostream>
#include <vector>
using namespace std;

void DeadEndTest::build_transition_index() {
  // transition_index[v][value][value'] is a numbering scheme for
  // arcs in the domain transition graph for variable v.

  // More precisely: Let A be the set of arcs in DTG_v (seen as an
  // unlabeled digraph, with at most one arc between any pair of nodes).
  // Then transition_index[v] is a bijection between A and the
  // set {0, 1, ..., |A| - 1}.

  // transition_index[v][value][value'] is -1 iff DTG_v has no arc
  // from value to value'.

  int var_count = g_variable_domain.size();
  transition_index.reserve(var_count);
  transition_counts.reserve(var_count);
  for(int v = 0; v < var_count; v++) {
    int dom = g_variable_domain[v];
    transition_index.push_back(vector<vector<int> >(dom, vector<int>(dom, -1)));
    // Now transition_index[v][value1][value2] == -1 for all value1, value2.
    int transition_number = 0;
    for(int from_value = 0; from_value < dom; from_value++) {
      vector<int> to_values;
      g_transition_graphs[v]->get_successors(from_value, to_values);
      for(int i = 0; i < to_values.size(); i++) {
	transition_index[v][from_value][to_values[i]] = transition_number;
	transition_number++;
      }
    }
    transition_counts.push_back(transition_number);
  }
}

void DeadEndTest::build_reachable_offsets() {
  int var_count = g_variable_domain.size();
  reachable_offsets.reserve(var_count);
  for(int v = 0; v < var_count; v++) {
    reachable_offsets.push_back(horn_variable_count);
    horn_variable_count += g_variable_domain[v];
  }
}

void DeadEndTest::build_cooccurs_offsets() {
  int var_count = g_variable_domain.size();
  cooccurs_offsets.reserve(var_count);
  for(int v1 = 0; v1 < var_count; v1++)
    cooccurs_offsets.push_back(vector<vector<int> >(v1));
  // Now cooccurs_offsets[v1][v2] is an empty vector<int> for all v1 > v2.

  for(int v1 = 0; v1 < var_count; v1++) {
    const vector<int> &related_vars = g_causal_graph->get_neighbours(v1);
    int dom1 = g_variable_domain[v1];
    for(int i = 0; i < related_vars.size(); i++) {
      int v2 = related_vars[i];
      if(v1 > v2) { // Each edge is visited twice, we only look at this direction.
	int dom2 = g_variable_domain[v2];
	cooccurs_offsets[v1][v2].reserve(dom1);
	for(int value1 = 0; value1 < dom1; value1++) {
	  cooccurs_offsets[v1][v2].push_back(horn_variable_count);
	  horn_variable_count += dom2;
	}
      }
    }
  }
}

void DeadEndTest::build_propagate_offsets() {
  int var_count = g_variable_domain.size();
  propagate_offsets.resize(var_count, vector<int>(var_count, -1));
  // Now propagate_offsets[v][v'] == -1 for all v, v'

  for(int v1 = 0; v1 < var_count; v1++) {
    int transition_count = transition_counts[v1];
    const vector<int> &related_vars = g_causal_graph->get_neighbours(v1);
    for(int i = 0; i < related_vars.size(); i++) {
      propagate_offsets[v1][related_vars[i]] = horn_variable_count;
      horn_variable_count += transition_count;
    }
  }
}

void DeadEndTest::count_rules() {
  // Init rule
  horn_rule_count = 1;

  // Propagation rules
  int var_count = g_variable_domain.size();
  for(int var = 0; var < var_count; var++) {
    int related_values = 0;
    const vector<int> &related_vars = g_causal_graph->get_neighbours(var);
    for(int i = 0; i < related_vars.size(); i++)
      related_values += g_variable_domain[related_vars[i]];
    horn_rule_count += related_values * transition_counts[var];
  }

  // Transition rules
  for(int i = 0; i < g_operators.size(); i++) {
    const vector<PrePost> &pre_post = g_operators[i].get_pre_post();
    for(int j = 0; j < pre_post.size(); j++) {
      if(pre_post[j].pre != -1)
	horn_rule_count += 1;
      else
	horn_rule_count += g_variable_domain[pre_post[j].var] - 1;
    }
  }
}

void DeadEndTest::build_init_rule() {
  cout << "Building initial state rule..." << endl;
  int var_count = g_variable_domain.size();
  HornRule *rule = horn->new_rule();
  for(int v = 0; v < var_count; v++) {
    rule->add_effect(reachable_var(v, (*g_initial_state)[v]));
    const vector<int> &neighbours = g_causal_graph->get_neighbours(v);
    for(int i = 0; i < neighbours.size(); i++) {
      int v2 = neighbours[i];
      if(v > v2)
	rule->add_effect(cooccurs_var(v, (*g_initial_state)[v],
					    v2, (*g_initial_state)[v2]));
    }
  }
}

void DeadEndTest::build_propagation_rules() {
  cout << "Building propagation rules..." << endl;
  int var_count = g_variable_domain.size();
  for(int parent = 0; parent < var_count; parent++) {
    for(int old_val = 0; old_val < g_variable_domain[parent]; old_val++) {
      vector<int> new_vals;
      g_transition_graphs[parent]->get_successors(old_val, new_vals);
      for(int i = 0; i < new_vals.size(); i++) {
	int new_val = new_vals[i];
	const vector<int> &related_vars = g_causal_graph->get_neighbours(parent);
	for(int j = 0; j < related_vars.size(); j++) {
	  int child = related_vars[j];
	  HornVariable *propagate_atom = propagate_var(parent, old_val, new_val, child);
	  for(int cval = 0; cval < g_variable_domain[child]; cval++) {
	    HornRule *rule = horn->new_rule();
	    rule->add_condition(propagate_atom);
	    rule->add_condition(cooccurs_var(parent, old_val, child, cval));
	    rule->add_effect(cooccurs_var(parent, new_val, child, cval));
	  }
	}
      }
    }
  }
}

void DeadEndTest::build_transition_rules() {
  cout << "Building rules for operators..." << endl;

  int var_count = g_variable_domain.size();
  propagate_vars.resize(var_count, false);

  for(int i = 0; i < g_operators.size(); i++) {
    Operator &op = g_operators[i];
    const vector<Prevail> &prev = op.get_prevail();
    const vector<PrePost> &pre_post = op.get_pre_post();

    vector<int> condition_vars, condition_vals;
    for(int j = 0; j < prev.size(); j++) {
      condition_vars.push_back(prev[j].var);
      condition_vals.push_back(prev[j].prev);
    }
    for(int j = 0; j < pre_post.size(); j++)
      if(pre_post[j].pre != -1) {
	condition_vars.push_back(pre_post[j].var);
	condition_vals.push_back(pre_post[j].pre);
      }

    for(int j = 0; j < pre_post.size(); j++) {
      int effect_var = pre_post[j].var;
      int effect_pre = pre_post[j].pre;
      int effect_post = pre_post[j].post;

      if(effect_pre != -1) {
	build_transition_rule(condition_vars, condition_vals,
			      effect_var, effect_pre, effect_post);
      } else {
	for(int k = 0; k < g_variable_domain[effect_var]; k++)
	  if(k != effect_post)
	    build_transition_rule(condition_vars, condition_vals,
				  effect_var, k, effect_post);
      }
    }
  }
}

void DeadEndTest::build_transition_rule(const vector<int> &cond_vars,
					const vector<int> &cond_vals,
					int effect_var, int effect_pre,
					int effect_post) {
  assert(effect_var >= 0 && effect_var < g_variable_domain.size());
  assert(effect_pre >= 0 && effect_pre < g_variable_domain[effect_var]);
  assert(effect_post >= 0 && effect_post < g_variable_domain[effect_var]);

  // Invariant: propagate_vars == vector<bool>(var_count, false)

  HornRule *rule = horn->new_rule();

  const vector<int> &related_vars = g_causal_graph->get_neighbours(effect_var);
  for(int i = 0; i < related_vars.size(); i++)
    propagate_vars[related_vars[i]] = true;

  rule->add_condition(reachable_var(effect_var, effect_pre));
  for(int i = 0; i < cond_vars.size(); i++) {
    int cvar = cond_vars[i], cval = cond_vals[i];
    if(cvar != effect_var)
      rule->add_condition(cooccurs_var(effect_var, effect_pre, cvar, cval));
  }

  rule->add_effect(reachable_var(effect_var, effect_post));
  for(int i = 0; i < cond_vars.size(); i++) {
    int cvar = cond_vars[i], cval = cond_vals[i];
    if(cvar != effect_var) {
      rule->add_effect(cooccurs_var(effect_var, effect_post, cvar, cval));
      propagate_vars[cvar] = false;
    }
  }

  for(int i = 0; i < related_vars.size(); i++) {
    int v = related_vars[i];
    if(propagate_vars[v]) {
      propagate_vars[v] = false;
      rule->add_effect(propagate_var(effect_var, effect_pre, effect_post, v));
    }
  }
}

DeadEndTest::DeadEndTest() {
  cout << "Initializing variables and rules..." << endl;
  build_transition_index();
  horn_variable_count = 0;
  build_reachable_offsets();
  build_cooccurs_offsets();
  build_propagate_offsets();
  count_rules();
  horn = new Horn(horn_variable_count, horn_rule_count);
}

DeadEndTest::~DeadEndTest() {
  delete horn;
}

bool DeadEndTest::is_reachable() {
  cout << "Variables: " << horn_variable_count << endl;
  cout << "Rules: " << horn_rule_count << endl;

  build_init_rule();
  build_propagation_rules();
  build_transition_rules();

  int model_size = horn->calculate_minimal_model();
  cout << "Minimal model size: " << model_size << endl;

  for(int i = 0; i < g_goal.size(); i++)
    if(!reachable_var(g_goal[i].first, g_goal[i].second)->is_true())
      return false;

  return true;
}
