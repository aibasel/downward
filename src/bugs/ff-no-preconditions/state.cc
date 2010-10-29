#include "axioms.h"
#include "ff_heuristic.h"
#include "globals.h"
#include "heuristic.h"
#include "operator.h"
#include "state.h"

#include <algorithm>
#include <iostream>
#include <cassert>
using namespace std;

State::State(istream &in) {
  check_magic(in, "begin_state");
  for(int i = 0; i < g_variable_domain.size(); i++) {
    int var;
    cin >> var;
    vars.push_back(var);
  }
  check_magic(in, "end_state");

  g_default_axiom_values = vars;
}

State::State(const State &predecessor, const Operator &op)
  : vars(predecessor.vars) {
  assert(!op.is_axiom());

  // Update values affected by operator.
  for(int i = 0; i < op.get_pre_post().size(); i++) {
    const PrePost &pre_post = op.get_pre_post()[i];
    if(pre_post.does_fire(predecessor))
      vars[pre_post.var] = pre_post.post;
  }

  g_axiom_evaluator->evaluate(*this);
}

int State::heuristic(int goal_max) {
  Heuristic heur(*this);
  return heur.get_heuristic(goal_max);
}

void State::heuristic_and_helpful_actions(int &cg_h, int &ff_h,
					  vector<const Operator *> &helpful_actions,
					  int goal_max) {

  assert(helpful_actions.empty());
  Heuristic heur(*this);
  cg_h = heur.get_heuristic(goal_max);

  if(g_multi_heuristic) {
    g_ff_heuristic->set_state(this);
    ff_h = g_ff_heuristic->get_ff_heuristic();
    if(ff_h == -1) {
      cg_h = -1;
      return;
    }
  } else {
    ff_h = -2;
  }

  if(cg_h != 0 && cg_h != -1) {
    heur.get_helpful_actions(helpful_actions, goal_max);
    if(helpful_actions.empty() && (g_ff_helpful_actions || g_mixed_helpful_actions)) {
      // HACK: This is how the FF heuristic signals dead ends.
      cg_h = -1;
      ff_h = -1;
    }
  }
}

void State::dump() const {
  for(int i = 0; i < vars.size(); i++)
    cout << "  " << g_variable_name[i] << ": " << vars[i] << endl;
}

bool State::operator<(const State &other) const {
  return lexicographical_compare(vars.begin(), vars.end(),
				 other.vars.begin(), other.vars.end());
}
