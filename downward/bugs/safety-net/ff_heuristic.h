#ifndef FF_HEURISTIC_H
#define FF_HEURISTIC_H

#include <vector>
using namespace std;

class Operator;
class State;

class Proposition;
class UnaryOperator;

struct UnaryOperator {
  const Operator *op;  // FF helpful actions only
  vector<Proposition *> precondition;
  Proposition *effect;

  int unsatisfied_preconditions;
  int h1plus_cost;
  int base_cost; // 0 for axioms, 1 for regular operators
  UnaryOperator(const vector<Proposition *> &pre, Proposition *eff,
		const Operator *the_op, int base)
    : precondition(pre), effect(eff), op(the_op), base_cost(base) {}
};

struct Proposition {
  bool is_goal;
  vector<UnaryOperator *> precondition_of;

  int h1plus_cost;
  UnaryOperator *reached_by;   // FF only

  Proposition() {
    is_goal = false;
    h1plus_cost = -1;
    reached_by = 0;
  }
};

class FFHeuristic {
  vector<UnaryOperator> unary_operators;
  vector<vector<Proposition> > propositions;
  vector<Proposition *> goal_propositions; // FF only

  const State *state;
  Proposition **reachable_queue_start;
  Proposition **reachable_queue_read_pos;
  Proposition **reachable_queue_write_pos;

  void build_unary_operators(const Operator &op);
  void simplify();

  int h1plus();
  int ff();
  void collect_relaxed_plan(Proposition *goal, vector<UnaryOperator *> &relaxed_plan);
  void extract_helpful_actions(Proposition *goal, vector<const Operator *> &result);
public:
  FFHeuristic();
  ~FFHeuristic();
  void set_state(const State *s);
  int get_h1plus_heuristic();
  int get_ff_heuristic();
  void get_helpful_actions(vector<const Operator *> &result);
};

#endif
