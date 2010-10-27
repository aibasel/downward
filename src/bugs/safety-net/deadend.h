#ifndef DEADEND_H
#define DEADEND_H

#include "globals.h"
#include "horn.h"

#include <vector>
using namespace std;

class DeadEndTest {
  Horn *horn;
  int horn_variable_count;
  int horn_rule_count;

  vector<int> transition_counts; // number of transitions of a given variable
  vector<bool> propagate_vars;   // frame mask used in add_rule_for_transition

  // Index tables for finding variable numbers.
  vector<int> reachable_offsets;
  vector<vector<vector<int> > > cooccurs_offsets;
  vector<vector<int> > propagate_offsets;
  vector<vector<vector<int> > > transition_index;

  void build_transition_index();
  void build_reachable_offsets();
  void build_cooccurs_offsets();
  void build_propagate_offsets();

  // Access functions for horn variables.
  HornVariable *var(int var_no) {
    return horn->get_variable(var_no);
  }

  HornVariable *reachable_var(int v, int value) {
    assert(v >= 0 && v < g_variable_domain.size());
    assert(value >= 0 && value < g_variable_domain[v]);
    return var(reachable_offsets[v] + value);
  }

  HornVariable *cooccurs_var(int v1, int value1, int v2, int value2) {
    if(v1 < v2) // Must have v1 > v2.
      return cooccurs_var(v2, value2, v1, value1);
    assert(v1 != v2);
    assert(v1 >= 0 && v1 < g_variable_domain.size());
    assert(value1 >= 0 && value1 < g_variable_domain[v1]);
    assert(v2 >= 0 && v2 < g_variable_domain.size());
    assert(value2 >= 0 && value2 < g_variable_domain[v2]);
    assert(cooccurs_offsets[v1][v2].size() == g_variable_domain[v1]);
    return var(cooccurs_offsets[v1][v2][value1] + value2);
  }

  HornVariable *propagate_var(int v1, int old_value, int new_value, int v2) {
    assert(v1 != v2 && old_value != new_value);
    assert(v1 >= 0 && v1 < g_variable_domain.size());
    assert(old_value >= 0 && old_value < g_variable_domain[v1]);
    assert(v2 >= 0 && v2 < g_variable_domain.size());
    assert(new_value >= 0 && new_value < g_variable_domain[v1]);
    assert(propagate_offsets[v1][v2] != -1);
    assert(transition_index[v1][old_value][new_value] != -1);
    return var(propagate_offsets[v1][v2] + transition_index[v1][old_value][new_value]);
  }

  // Methods for counting and building rules.
  void count_rules();
  void build_init_rule();
  void build_propagation_rules();
  void build_transition_rules();
  void build_transition_rule(const vector<int> &cond_vars, const vector<int> &cond_vals,
			     int effect_var, int effect_pre, int effect_post);
public:
  DeadEndTest();
  ~DeadEndTest();
  bool is_reachable();
};

#endif
