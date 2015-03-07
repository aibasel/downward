#ifndef SEARCH_PROGRESS_H
#define SEARCH_PROGRESS_H

#include "heuristic.h"

#include <vector>

class EvaluationContext;


/*
  This class helps track search progress.

  It keeps counters for expanded, generated and evaluated states (and
  some other statistics) and provides uniform output for all search
  methods.
*/

class SearchProgress {
    // General statistics
    int expanded_states;  // no states for which successors were generated
    int evaluated_states; // no states for which h fn was computed
    int evaluations;      // no of heuristic evaluations performed
    int generated_states; // no states created in total (plus those removed since already in close list)
    int reopened_states;  // no of *closed* states which we reopened
    int dead_end_states;

    int generated_ops;    // no of operators that were returned as applicable
    int pathmax_corrections; // no of pathmax corrections;

    // Statistics related to f values
    int lastjump_f_value; //f value obtained in the last jump
    int lastjump_expanded_states; // same guy but at point where the last jump in the open list
    int lastjump_reopened_states; // occurred (jump == f-value of the first node in the queue increases)
    int lastjump_evaluated_states;
    int lastjump_generated_states;

    // Heuristics for which we collect statistics
    std::vector<Heuristic *> heuristics;

    // Statistics related to h values
    std::vector<int> initial_heuristic_values;  // heuristic values of the initial state
    std::vector<int> best_heuristic_values;     // best heuristic values so far
public:
    SearchProgress();
    ~SearchProgress();

    // Methods that update statistics
    void inc_expanded(int inc = 1) {expanded_states += inc; }
    void inc_evaluated_states(int inc = 1) {evaluated_states += inc; }
    void inc_generated(int inc = 1) {generated_states += inc; }
    void inc_reopened(int inc = 1) {reopened_states += inc; }
    void inc_generated_ops(int inc = 1) {generated_ops += inc; }
    void inc_pathmax_corrections(int inc = 1) {pathmax_corrections += inc; }
    void inc_evaluations(int inc = 1) {evaluations += inc; }
    void inc_dead_ends(int inc = 1) {dead_end_states += inc; }

    // Methods that access statistics
    int get_expanded() const {return expanded_states; }
    int get_evaluated_states() const {return evaluated_states; }
    int get_evaluations() const {return evaluations; }
    int get_generated() const {return generated_states; }
    int get_reopened() const {return reopened_states; }
    int get_generated_ops() const {return generated_ops; }
    int get_pathmax_corrections() const {return pathmax_corrections; }

    // Methods related to heuristic values

    void add_heuristic(Heuristic *heur);

    /*
      Call the following method with the f value of every expanded
      state. It will notice "jumps" (i.e., when the expanded f value
      is the highest f value encountered so far), print some
      statistics on jumps, and keep track of expansions etc. up to the
      last jump.

      Statistics until the final jump are often useful to report in
      A*-style searches because they are not affected by tie-breaking
      as the overall statistics. (With a non-random, admissible and
      consistent heuristic, the number of expanded, evaluated and
      generated states until the final jump is fully determined by the
      state space and heuristic, independently of things like the
      order in which successors are generated or the tie-breaking
      performed by the open list.)
    */
    void report_f_value(int f);

    /*
      Call the following function with the heuristic evaluation
      results of the initial state. Calling this function is mandatory
      before calling other methods that keep track of heuristic
      values.

      TODO: Logically EvaluationContext should be "const" here, but we
      don't currently have a good const interface for
      EvaluationContext.
    */
    void set_initial_h_values(EvaluationContext &eval_context);

    /*
      Call this function with the heuristic evaluation results of an
      evaluated state. All calls to set_initial_h_value and
      check_h_progress must use the same heuristics in the same order.

      The method keeps track of the lowest-ever value of each
      heuristic, returning true and producing statistics output if
      there is a new best value for any heuristic. The g value passed
      into this function is only used for output, so passing in a
      dummy value will not cause problems (other than ugly output).

      TODO: Logically EvaluationContext should be "const" here, but we
      don't currently have a good const interface for
      EvaluationContext.
    */
    bool check_h_progress(EvaluationContext &eval_context, int g);

    // output
    void print_line() const;
    void print_f_line() const;
    void print_best_heuristic_values(int g) const;
    void print_statistics() const;
};

#endif
