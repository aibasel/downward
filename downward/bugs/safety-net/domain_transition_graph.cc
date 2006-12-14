#include <iostream>
#include <map>
#include <cassert>
#include <ext/hash_map>
using namespace std;
using namespace __gnu_cxx;

#include "domain_transition_graph.h"
#include "globals.h"
#include "operator.h"

void DomainTransitionGraph::read_all(istream &in) {
  int var_count = g_variable_domain.size();

  // First step: Allocate graphs and nodes.
  g_transition_graphs.reserve(var_count);
  for(int var = 0; var < var_count; var++) {
    int range = g_variable_domain[var];
    DomainTransitionGraph *dtg = new DomainTransitionGraph(var, range);
    g_transition_graphs.push_back(dtg);
  }

  // Second step: Read transitions from file.
  for(int var = 0; var < var_count; var++)
    g_transition_graphs[var]->read_data(in);

  // Third step: Simplify transitions.
  // Don't do this for ADL domains, because the algorithm is exponential
  // in the number of conditions of a transition, which is a constant for STRIPS
  // domains, but not for ADL domains.

  cout << "Simplifying transitions..." << flush;
  for(int var = 0; var < var_count; var++) {
    vector<ValueNode> &nodes = g_transition_graphs[var]->nodes;
    for(int value = 0; value < nodes.size(); value++)
      for(int i = 0; i < nodes[value].transitions.size(); i++)
	nodes[value].transitions[i].simplify();
  }
  cout << " done!" << endl;

}

DomainTransitionGraph::DomainTransitionGraph(int var_index, int node_count) {
  is_axiom = g_axiom_layers[var_index] != -1;
  var = var_index;
  nodes.reserve(node_count);
  for(int value = 0; value < node_count; value++)
    nodes.push_back(ValueNode(this, value));
  last_helpful_action_extraction_time = -1;
}

void DomainTransitionGraph::read_data(istream &in) {
  check_magic(in, "begin_DTG");

  map<int, int> global_to_local_child;
  map<pair<int, int>, int> transition_index;
  // TODO: This transition index business is caused by the fact
  //       that transitions in the input are not grouped by target
  //       like they should be. Change this.

  for(int origin = 0; origin < nodes.size(); origin++) {
    int trans_count;
    in >> trans_count;
    for(int i = 0; i < trans_count; i++) {
      int target, operator_index;
      in >> target;
      in >> operator_index;

      pair<int, int> arc = make_pair(origin, target);
      if(!transition_index.count(arc)) {
	transition_index[arc] = nodes[origin].transitions.size();
	nodes[origin].transitions.push_back(ValueTransition(&nodes[target]));
      }

      assert(transition_index.count(arc));
      ValueTransition *transition = &nodes[origin].transitions[transition_index[arc]];

      vector<PrevailCondition> prevail;
      int prevail_count;
      in >> prevail_count;
      for(int j = 0; j < prevail_count; j++) {
	int global_var, val;
	in >> global_var >> val;
	if(global_var < var) { // [ignore cycles]
	  if(!global_to_local_child.count(global_var)) {
	    global_to_local_child[global_var] = local_to_global_child.size();
	    local_to_global_child.push_back(global_var);
	  }
	  int local_var = global_to_local_child[global_var];
	  DomainTransitionGraph *prev_dtg = g_transition_graphs[global_var];
	  prevail.push_back(PrevailCondition(prev_dtg, local_var, val));
	}
      }
      Operator *the_operator;
      if(is_axiom) {
	assert(operator_index >= 0 && operator_index < g_axioms.size());
	the_operator = &g_axioms[operator_index];
      } else {
	assert(operator_index >= 0 && operator_index < g_operators.size());
	the_operator = &g_operators[operator_index];
      }
      transition->labels.push_back(ValueTransitionLabel(the_operator, prevail));
    }
  }
  check_magic(in, "end_DTG");
}

void DomainTransitionGraph::dump() const {
  cout << "DomainTransitionGraph::dump() not implemented." << endl;
  assert(false);
}

void DomainTransitionGraph::get_successors(int value, vector<int> &result) const {
  assert(result.empty());
  assert(value >= 0 && value < nodes.size());
  const vector<ValueTransition> &transitions = nodes[value].transitions;
  result.reserve(transitions.size());
  for(int i = 0; i < transitions.size(); i++)
    result.push_back(transitions[i].target->value);
}

class hash_pair_vector {
public:
  size_t operator()(const vector<pair<int, int> > &vec) const {
    unsigned long hash_value = 0; 
    for(int i = 0; i < vec.size(); i++) {
      hash_value = 17 * hash_value + vec[i].first;
      hash_value = 17 * hash_value + vec[i].second;
    }
    return size_t(hash_value);
  }
};

void ValueTransition::simplify() {
  // Remove labels with duplicate or dominated conditions.

  /*
    Algorithm: Put all transitions into a hash_map
    (key: condition, value: index in label list).
    This already gets rid of transitions with identical conditions.

    Then go through the hash_map, checking for each element if
    none of the subset conditions are part of the hash_map.
    Put the element into the new labels list iff this is the case.
   */

  typedef vector<pair<int, int> > HashKey;
  typedef hash_map<HashKey, int, hash_pair_vector> HashMap;
  HashMap label_index;
  label_index.resize(labels.size() * 2);

  for(int i = 0; i < labels.size(); i++) {
    HashKey key;
    const vector<PrevailCondition> &conditions = labels[i].prevail;
    for(int j = 0; j < conditions.size(); j++)
      key.push_back(make_pair(conditions[j].local_var, conditions[j].value));
    sort(key.begin(), key.end());
    label_index[key] = i;
  }

  vector<ValueTransitionLabel> old_labels;
  old_labels.swap(labels);

  for(HashMap::iterator it = label_index.begin(); it != label_index.end(); ++it) {
    const HashKey &key = it->first;
    int label_no = it->second;
    int powerset_size = (1 << key.size()) - 1; // -1: only consider proper subsets
    bool match = false;
    if(powerset_size <= 31) { // HACK! Don't spend too much time here...
      for(int mask = 0; mask < powerset_size; mask++) {
	HashKey subset;
	for(int i = 0; i < key.size(); i++)
	  if(mask & (1 << i))
	    subset.push_back(key[i]);
	if(label_index.count(subset)) {
	  match = true;
	  break;
	}
      }
    }
    if(!match)
      labels.push_back(old_labels[label_no]);
  }
}

