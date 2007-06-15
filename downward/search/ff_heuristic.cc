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
  NOTE: Our implementation of h_add is actually a "poor man's h_add".
  Propositions which were already popped from the queue are not
  reevaluated. Since propositions are ordered by a queue, not by a heap,
  this means that some values might end up too high (which actually happens).
  Maybe it would pay off to use a proper heap, maybe not.
  More experiments are needed. In any case, this is comparatively slooow.
  Are simplifications possible?
  
  One possible way of doing this is grouping similar unary operators.
  (E.g. in Grid, the "hand status" aspects of pickup-and-loose and pickup
  don't actually need to define a precondition on the current status of the
  hand because *all* possible values are allowed.)
*/

// construction and destruction
FFHeuristic::FFHeuristic(bool use_cache)
    : Heuristic(use_cache) {
    reachable_queue_start = 0;
    reachable_queue_read_pos = 0;
    reachable_queue_write_pos = 0;
}

FFHeuristic::~FFHeuristic() {
    delete[] reachable_queue_start;
}

// initialization
void FFHeuristic::initialize() {
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

// heuristic computation
void FFHeuristic::setup_exploration_queue() {
    reachable_queue_read_pos = reachable_queue_start;
    reachable_queue_write_pos = reachable_queue_start;

    for(int var = 0; var < propositions.size(); var++) {
	for(int value = 0; value < propositions[var].size(); value++) {
	    Proposition &prop = propositions[var][value];
	    prop.h_add_cost = -1;
	}
    }

    // Deal with operators and axioms without preconditions.
    for(int i = 0; i < unary_operators.size(); i++) {
	UnaryOperator &op = unary_operators[i];
	op.unsatisfied_preconditions = op.precondition.size();
	op.h_add_cost = op.base_cost; // will be increased by precondition costs

	if(op.unsatisfied_preconditions == 0) {
	    if(op.effect->h_add_cost == -1) {
		*reachable_queue_write_pos++ = op.effect;
		op.effect->h_add_cost = op.base_cost;
	    } else if(op.effect->h_add_cost > op.base_cost) {
		op.effect->h_add_cost = op.base_cost;
	    }
	}
    }
}

void FFHeuristic::setup_exploration_queue_state(const State &state) {
    for(int var = 0; var < propositions.size(); var++) {
	Proposition *init_prop = &propositions[var][state[var]];
	if(init_prop->h_add_cost == -1)
	    *reachable_queue_write_pos++ = init_prop;
	init_prop->h_add_cost = 0;
	init_prop->reached_by = 0; // only needed for FF heuristic
    }
}

void FFHeuristic::relaxed_exploration() {
    int unsolved_goals = goal_propositions.size();
    while(reachable_queue_read_pos != reachable_queue_write_pos) {
	Proposition *prop = *reachable_queue_read_pos++;
	int prop_cost = prop->h_add_cost;
	if(prop->is_goal && --unsolved_goals == 0)
	    break;
	const vector<UnaryOperator *> &triggered_operators = prop->precondition_of;
	for(int i = 0; i < triggered_operators.size(); i++) {
	    UnaryOperator *unary_op = triggered_operators[i];
	    unary_op->unsatisfied_preconditions--;
	    unary_op->h_add_cost += prop_cost;
	    assert(unary_op->unsatisfied_preconditions >= 0);
	    if(unary_op->unsatisfied_preconditions == 0) {
		Proposition *effect = unary_op->effect;
		if(effect->h_add_cost == -1) {
		    // Proposition reached for the first time: put into queue
		    effect->h_add_cost = unary_op->h_add_cost;
		    effect->reached_by = unary_op;
		    *reachable_queue_write_pos++ = effect;
		} else if(unary_op->h_add_cost < effect->h_add_cost) {
		    effect->h_add_cost = unary_op->h_add_cost;
		    effect->reached_by = unary_op;
		}
	    }
	}
    }
}

int FFHeuristic::compute_hsp_add_heuristic() {
    relaxed_exploration();

    int total_cost = 0;
    for(int i = 0; i < goal_propositions.size(); i++) {
	int prop_cost = goal_propositions[i]->h_add_cost;
	if(prop_cost == -1)
	    return DEAD_END;
	total_cost += prop_cost;
    }
    return total_cost;
}

int FFHeuristic::compute_ff_heuristic() {
    int h_add_heuristic = compute_hsp_add_heuristic();
    if(h_add_heuristic == DEAD_END) {
	return DEAD_END;
    } else {
	RelaxedPlan relaxed_plan;
	relaxed_plan.resize(2 * h_add_heuristic);
	// Collecting the relaxed plan also marks helpful actions as preferred.
	for(int i = 0; i < goal_propositions.size(); i++)
	    collect_relaxed_plan(goal_propositions[i], relaxed_plan);
	return relaxed_plan.size();
    }
}

void FFHeuristic::collect_relaxed_plan(Proposition *goal,
				       RelaxedPlan &relaxed_plan) {
    UnaryOperator *unary_op = goal->reached_by;
    if(unary_op) { // We have not yet chained back to a start node.
	for(int i = 0; i < unary_op->precondition.size(); i++)
	    collect_relaxed_plan(unary_op->precondition[i], relaxed_plan);
	const Operator *op = unary_op->op;
	bool added_to_relaxed_plan = relaxed_plan.insert(op).second;
	if(added_to_relaxed_plan
	   && unary_op->h_add_cost == unary_op->base_cost
	   && !op->is_axiom()) {
	    set_preferred(op); // This is a helpful action.
	}
    }
}

int FFHeuristic::compute_heuristic(const State &state) {
    setup_exploration_queue();
    setup_exploration_queue_state(state);
    return compute_ff_heuristic();
}

