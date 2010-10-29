/*
  Horn::minimal_model() evaluates a list of (compressed) Horn Rules of the
  form
                     A and B and C ... -> X and Y and Z ...
  The left hand side may be an empty list, the right hand side may not.
  That is, we do not deal with full Horn Logic but only with positive Horn
  clauses. It is easy to add support for negative Horn clauses: Fail whenever
  a rule with empty right hand side fires.

  The algorithm calculates the set of variables which must be set to True
  for *all* satisfying assignments, the "minimal model" of the formula.
*/

#include "horn.h"

#include <cassert>
using namespace std;

Horn::Horn(int variable_count, int rule_count)
  : variables(variable_count), rules(rule_count) {
  next_rule = rules.begin();
}

int Horn::calculate_minimal_model() {
  // Markierungsalgorithmus (pebble algorithm)
  // cf. Schöning: Logik für Informatiker (4. Auflage), S. 33.

  assert(next_rule == rules.end());

  int variable_count = variables.size();
  int rule_count = rules.size();

  vector<HornVariable *> queue;
  queue.clear();
  queue.reserve(variable_count);

  for(int rule_no = 0; rule_no < rule_count; rule_no++) {
    HornRule &rule = rules[rule_no];
    if(rule.unsatisfied_conditions == 0) {
      const vector<HornVariable *> &effects = rule.effects;
      for(int i = 0; i < effects.size(); i++) {
	HornVariable *var = effects[i];
	if(var->truth_value == false) {
	  var->truth_value = true;
	  queue.push_back(var);
	}
      }
    }
  }

  for(int queue_index = 0; queue_index < queue.size(); queue_index++) {
    HornVariable *var = queue[queue_index];
    const vector<HornRule *> &triggered_rules = var->condition_of;
    for(int i = 0; i < triggered_rules.size(); i++) {
      HornRule &rule = *triggered_rules[i];
      rule.unsatisfied_conditions--;
      assert(rule.unsatisfied_conditions >= 0);

      // The following is the same as the block above, but we do not trust
      // the inlining decisions of the compiler. This *must* be inlined for speed.
      if(rule.unsatisfied_conditions == 0) {
	const vector<HornVariable *> &effects = rule.effects;
	for(int i = 0; i < effects.size(); i++) {
	  HornVariable *var = effects[i];
	  if(var->truth_value == false) {
	    var->truth_value = true;
	    queue.push_back(var);
	  }
	}
      }
    }
  }

  return queue.size();
}
