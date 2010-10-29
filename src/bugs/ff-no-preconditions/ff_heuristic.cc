#include "ff_heuristic.h"
#include "globals.h"
#include "operator.h"
#include "state.h"

#include <cassert>
#include <vector>
using namespace std;

#include <ext/hash_map>
using namespace __gnu_cxx;

/*
  NOTE: Our implementation of h1+ is actually a "poor man's h1+".
  Propositions which were already popped from the queue are not
  reevaluated. Since propositions are ordered by a queue, not by a heap,
  this means that some values might end up too high (which actually happens).
  Maybe it would pay off to use a proper heap, maybe not.
  More experiments are needed. In any case, this is comparatively slooow.
  Are simplifications possible?
  
  If so, then probably only by grouping similar unary predicates.
  (E.g. in Grid, the "hand status" aspects of pickup-and-loose and pickup
  don't actually need to define a precondition on the current status of the
  hand because *all* possible values are allowed.)
*/

/*
TODO:
  There's no point in using unary operators (instead of ordinary operators)
  for the FF and h1+ heuristics, except that this is the information present
  in the input.

  For h1+, unary operators don't hurt -- the results are identical.
  However, for FF this can be a problem and should be changed.
*/

void FFHeuristic::build_unary_operators(const Operator &op) {
  int base_cost = op.is_axiom() ? 0 : 1;
  const vector<Prevail> &prevail = op.get_prevail();
  const vector<PrePost> &pre_post = op.get_pre_post();
  vector<Proposition *> precondition;
  for(int i = 0; i < prevail.size(); i++) {
    assert(prevail[i].var >= 0 && prevail[i].var < g_variable_domain.size());
    assert(prevail[i].prev >= 0 && prevail[i].prev < g_variable_domain[prevail[i].var]);
    precondition.push_back(&propositions[prevail[i].var][prevail[i].prev]);
  }
  for(int i = 0; i < pre_post.size(); i++)
    if(pre_post[i].pre != -1) {
      assert(pre_post[i].var >= 0 && pre_post[i].var < g_variable_domain.size());
      assert(pre_post[i].pre >= 0 && pre_post[i].pre < g_variable_domain[pre_post[i].var]);
      precondition.push_back(&propositions[pre_post[i].var][pre_post[i].pre]);
    }
  for(int i = 0; i < pre_post.size(); i++) {
    assert(pre_post[i].var >= 0 && pre_post[i].var < g_variable_domain.size());
    assert(pre_post[i].post >= 0 && pre_post[i].post < g_variable_domain[pre_post[i].var]);
    Proposition *effect = &propositions[pre_post[i].var][pre_post[i].post];
    const vector<Prevail> &eff_cond = pre_post[i].cond;
    for(int j = 0; j < eff_cond.size(); j++) {
      assert(eff_cond[j].var >= 0 && eff_cond[j].var < g_variable_domain.size());
      assert(eff_cond[j].prev >= 0 && eff_cond[j].prev < g_variable_domain[eff_cond[j].var]);
      precondition.push_back(&propositions[eff_cond[j].var][eff_cond[j].prev]);
    }
    unary_operators.push_back(UnaryOperator(precondition, effect, &op, base_cost));
    precondition.erase(precondition.end() - eff_cond.size(), precondition.end());
  }
}
  
FFHeuristic::FFHeuristic() {
  assert(g_ff_helpful_actions || g_mixed_helpful_actions || g_multi_heuristic);

  cout << "Initializing HSP/FF heuristic..." << endl;

  // Build propositions.
  for(int var = 0; var < g_variable_domain.size(); var++)
    propositions.push_back(vector<Proposition>(g_variable_domain[var]));

  // Build goal propositions.
  for(int i = 0; i < g_goal.size(); i++) {
    int var = g_goal[i].first, val = g_goal[i].second;
    propositions[var][val].is_goal = true;
    goal_propositions.push_back(&propositions[var][val]);
  }

  // Build unary operators for operators and axioms.
  for(int i = 0; i < g_operators.size(); i++)
    build_unary_operators(g_operators[i]);
  for(int i = 0; i < g_axioms.size(); i++)
    build_unary_operators(g_axioms[i]);

  // Simplify unary operators.
  simplify();

  // Cross-reference unary operators.
  for(int i = 0; i < unary_operators.size(); i++) {
    UnaryOperator *op = &unary_operators[i];
    for(int j = 0; j < op->precondition.size(); j++)
      op->precondition[j]->precondition_of.push_back(op);
  }

  // Allocate reachability queue.
  int total_proposition_no = 0;
  for(int i = 0; i < propositions.size(); i++)
    total_proposition_no += propositions[i].size();
  reachable_queue_start = new Proposition *[total_proposition_no];
}

FFHeuristic::~FFHeuristic() {
  delete[] reachable_queue_start;
}

void FFHeuristic::set_state(const State *s) {
  state = s;

  reachable_queue_read_pos = reachable_queue_start;
  reachable_queue_write_pos = reachable_queue_start;

  for(int var = 0; var < propositions.size(); var++) {
    for(int value = 0; value < propositions[var].size(); value++) {
      Proposition &prop = propositions[var][value];
      prop.h1plus_cost = -1;
    }
    Proposition *init_prop = &propositions[var][(*state)[var]];
    init_prop->h1plus_cost = 0;
    init_prop->reached_by = 0; // only needed for FF heuristic
    *reachable_queue_write_pos++ = init_prop;
  }

  // Deal with operators and axioms without preconditions.
  for(int i = 0; i < unary_operators.size(); i++) {
    UnaryOperator &op = unary_operators[i];
    op.unsatisfied_preconditions = op.precondition.size();
    op.h1plus_cost = op.base_cost; // will be increased by precondition costs

    if(op.unsatisfied_preconditions == 0) {
      if(op.effect->h1plus_cost == -1) {
	*reachable_queue_write_pos++ = op.effect;
	op.effect->h1plus_cost = op.base_cost;
      } else if(op.effect->h1plus_cost > op.base_cost) {
	op.effect->h1plus_cost = op.base_cost;
      }
    }
  }
}

int FFHeuristic::get_h1plus_heuristic() {
  int unsolved_goals = goal_propositions.size();
  int goal_cost = 0;
  while(reachable_queue_read_pos != reachable_queue_write_pos) {
    Proposition *prop = *reachable_queue_read_pos++;
    int prop_cost = prop->h1plus_cost;
    if(prop->is_goal) {
      goal_cost += prop_cost;
      unsolved_goals--;
      if(unsolved_goals == 0)
	return goal_cost;
    }
    const vector<UnaryOperator *> &triggered_operators = prop->precondition_of;
    for(int i = 0; i < triggered_operators.size(); i++) {
      UnaryOperator *unary_op = triggered_operators[i];
      unary_op->unsatisfied_preconditions--;
      unary_op->h1plus_cost += prop_cost;
      assert(unary_op->unsatisfied_preconditions >= 0);
      if(unary_op->unsatisfied_preconditions == 0) {
	Proposition *effect = unary_op->effect;
	if(effect->h1plus_cost == -1) {
	  // Proposition reached for the first time: put into queue
	  effect->h1plus_cost = unary_op->h1plus_cost;
	  effect->reached_by = unary_op;
	  *reachable_queue_write_pos++ = effect;
	} else if(unary_op->h1plus_cost < effect->h1plus_cost) {
	  effect->h1plus_cost = unary_op->h1plus_cost;
	  effect->reached_by = unary_op;
	}
      }
    }
  }
  return -1;
}

int FFHeuristic::get_ff_heuristic() {
  int h1plus_heuristic = get_h1plus_heuristic();
  if(h1plus_heuristic <= 0) // 0 and -1 (unsolvable) must be special cased
    return h1plus_heuristic;
  
  vector<UnaryOperator *> relaxed_plan;
  relaxed_plan.reserve(h1plus_heuristic);
  for(int i = 0; i < goal_propositions.size(); i++)
    collect_relaxed_plan(goal_propositions[i], relaxed_plan);
  // assert(relaxed_plan.size() == h1plus_heuristic);
  
  // TODO: Hashing or even just marking the operators as they are used
  // and counting the marks as they are removed would be faster.
  // However, this does not look like a bottleneck, so we defer this.
  sort(relaxed_plan.begin(), relaxed_plan.end());
  int ff_heuristic = 1;
  for(int i = 1; i < relaxed_plan.size(); i++)
    if(relaxed_plan[i] != relaxed_plan[i - 1])
      ff_heuristic++;
  return ff_heuristic;
}

void FFHeuristic::collect_relaxed_plan(Proposition *goal,
				       vector<UnaryOperator *> &relaxed_plan) {
  UnaryOperator *op = goal->reached_by;
  if(op) { // We have not yet chained back to a start node.
    for(int i = 0; i < op->precondition.size(); i++)
      collect_relaxed_plan(op->precondition[i], relaxed_plan);
    relaxed_plan.push_back(op);
  }
}

void FFHeuristic::get_helpful_actions(vector<const Operator *> &result) {
  // NOTE:
  // Unlike helpful CG actions, this assumes that we have *not* yet
  // computed the FF/h1+ heuristic.

  // HACK! Must initialize iff we did not properly compute the FF heuristic yet.
  if(!g_multi_heuristic) {
    int h1plus = get_h1plus_heuristic(); // initializes "reached_by" chains.
    if(h1plus == -1) {
      // HACK: Clear result array to indicate unsolvability.
      result.clear();
      return;
    }
  }

  for(int i = 0; i < goal_propositions.size(); i++)
    extract_helpful_actions(goal_propositions[i], result);
  assert(!result.empty());
}

void FFHeuristic::extract_helpful_actions(Proposition *goal,
					  vector<const Operator *> &helpful_actions) {
  UnaryOperator *op = goal->reached_by;
  if(op) { // We have not yet chained back to a start node.
    int count = 0;
    for(int i = 0; i < op->precondition.size(); i++) {
      if(op->precondition[i]->reached_by == 0)
	count++;
      else
	extract_helpful_actions(op->precondition[i], helpful_actions);
    }
    if(count == op->precondition.size()) {
      assert(!op->op->is_axiom());
      helpful_actions.push_back(op->op);
    }
  }
}

class hash_unary_operator {
public:
  size_t operator()(const pair<vector<Proposition *>, Proposition *> &key) const {
    unsigned long hash_value = reinterpret_cast<unsigned long>(key.second); 
    const vector<Proposition *> &vec = key.first;
    for(int i = 0; i < vec.size(); i++)
      hash_value = 17 * hash_value + reinterpret_cast<unsigned long>(vec[i]);
    return size_t(hash_value);
  }
};

void FFHeuristic::simplify() {
  // Remove duplicate or dominated unary operators.

  /*
    Algorithm: Put all unary operators into a hash_map
    (key: condition and effect; value: index in operator vector.
    This gets rid of operators with identical conditions.

    Then go through the hash_map, checking for each element if
    none of the possible dominators are part of the hash_map.
    Put the element into the new operator vector iff this is the case.
   */

  cout << "Simplifying " << unary_operators.size() << " unary operators..." << flush;

  typedef pair<vector<Proposition *>, Proposition *> HashKey;
  typedef hash_map<HashKey, int, hash_unary_operator> HashMap;
  HashMap unary_operator_index;
  unary_operator_index.resize(unary_operators.size() * 2);

  for(int i = 0; i < unary_operators.size(); i++) {
    UnaryOperator &op = unary_operators[i];
    sort(op.precondition.begin(), op.precondition.end());
    HashKey key(op.precondition, op.effect);
    unary_operator_index[key] = i;
  }

  vector<UnaryOperator> old_unary_operators;
  old_unary_operators.swap(unary_operators);

  for(HashMap::iterator it = unary_operator_index.begin();
      it != unary_operator_index.end(); ++it) {
    const HashKey &key = it->first;
    int unary_operator_no = it->second;
    int powerset_size = (1 << key.first.size()) - 1; // -1: only consider proper subsets
    bool match = false;
    if(powerset_size <= 31) { // HACK! Don't spend too much time here...
      for(int mask = 0; mask < powerset_size; mask++) {
	HashKey dominating_key = make_pair(vector<Proposition *>(), key.second);
	for(int i = 0; i < key.first.size(); i++)
	  if(mask & (1 << i))
	    dominating_key.first.push_back(key.first[i]);
	if(unary_operator_index.count(dominating_key)) {
	  match = true;
	  break;
	}
      }
    }
    if(!match)
      unary_operators.push_back(old_unary_operators[unary_operator_no]);
  }

  cout << " done! [" << unary_operators.size() << " unary operators]" << endl;
}
