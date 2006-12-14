#ifndef HEURISTIC_H
#define HEURISTIC_H

#include <vector>
using namespace std;

class DomainTransitionGraph;
class Operator;
class State;

class Heuristic {
  enum {QUITE_A_LOT = 1000000};
  
  const State &state;

  void cg_setup_domain_transition_graphs();
  int get_transition_cost(DomainTransitionGraph *dtg, int start_val, int goal_val);
  int cg(int goal_max);
  void extract_helpful_actions_cg(DomainTransitionGraph *dtg,
				  int to, vector<const Operator *> &result);
  void get_helpful_actions_cg(vector<const Operator *> &result, int goal_max);
public:
  Heuristic(const State &s);

  int get_heuristic(int goal_max = QUITE_A_LOT);
  void get_helpful_actions(vector<const Operator *> &result,
			   int goal_max = QUITE_A_LOT);
};

#endif
