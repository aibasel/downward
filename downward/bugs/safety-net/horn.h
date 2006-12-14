#ifndef HORN_H
#define HORN_H

#include <cassert>
#include <vector>
using namespace std;

class HornVariable;
class HornRule;
class Horn;

class HornVariable {
  friend class Horn;
  friend class HornRule;
  bool truth_value;
  vector<HornRule *> condition_of;
public:
  HornVariable() { // Should not be used by user code!
    truth_value = false;
  }
  bool is_true() const {
    return truth_value;
  }
};

class HornRule {
  friend class Horn;
  int unsatisfied_conditions;
  vector<HornVariable *> effects;
public:
  HornRule() { // Should not be used by user code!
    unsatisfied_conditions = 0;
  }
  void add_condition(HornVariable *var) {
    unsatisfied_conditions++;
    var->condition_of.push_back(this);
  }
  void add_effect(HornVariable *var) {
    effects.push_back(var);
  }
};

class Horn {
  vector<HornVariable> variables;
  vector<HornRule> rules;
  vector<HornRule>::iterator next_rule;
public:
  Horn(int variable_count, int rule_count);

  HornRule *new_rule() {
    assert(next_rule != rules.end());
    return &*next_rule++;
  }
  
  HornVariable *get_variable(int var_index) {
    return &variables[var_index];
  }

  int calculate_minimal_model(); // returns size of model
};

#endif
