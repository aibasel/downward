#ifndef SEARCH_PROGRESS_H
#define SEARCH_PROGRESS_H

#include "heuristic.h"

#include <vector>

class EvaluationContext;


/*
  This class helps track search progress.

  It maintains a vector of best heuristic values and can determine if
  a given state has a better heuristic value in at least one of the
  heueristics that all previously seen ones.
*/

class SearchProgress {
    std::vector<Heuristic *> heuristics;
    std::vector<int> initial_heuristic_values;
    std::vector<int> best_heuristic_values;

public:
    SearchProgress() = default;
    ~SearchProgress() = default;

    // Methods related to heuristic values
    void add_heuristic(Heuristic *heur);

    /*
      Call the following function with the heuristic evaluation
      results of the initial state. Calling this function is mandatory
      before calling other methods that keep track of heuristic
      values.
    */
    void set_initial_h_values(EvaluationContext &eval_context);

    /*
      Call this function with the heuristic evaluation results of an
      evaluated state. All calls to set_initial_h_value and
      check_h_progress must use the same heuristics in the same order.

      The method keeps track of the lowest-ever value of each
      heuristic, returning true and producing statistics output if
      there is a new best value for any heuristic.
    */
    bool check_h_progress(EvaluationContext &eval_context);

    // output
    void print_initial_h_values() const;
    void print_best_heuristic_values() const;
};

#endif
