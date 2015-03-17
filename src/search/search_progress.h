#ifndef SEARCH_PROGRESS_H
#define SEARCH_PROGRESS_H

#include "heuristic.h"

#include <vector>

class EvaluationContext;
class SearchStatistics;


/*
  This class helps track search progress.

  It maintains a vector of best heuristic values and can determine if
  a given state has a better heuristic value in at least one of the
  heueristics that all previously seen ones.
*/

class SearchProgress {
    // Heuristics for which we collect statistics
    std::vector<Heuristic *> heuristics;

    // Statistics related to h values
    std::vector<int> initial_heuristic_values;  // heuristic values of the initial state
    std::vector<int> best_heuristic_values;     // best heuristic values so far

    void print_best_heuristic_values(
        int g, const SearchStatistics &statistics) const;
public:
    SearchProgress() = default;
    ~SearchProgress() = default;

    // Methods related to heuristic values
    void add_heuristic(Heuristic *heur);

    /*
      Call the following function with the heuristic evaluation
      results of the initial state. Calling this function is mandatory
      before calling other methods that keep track of heuristic
      values. It also prints the initial h value (in the same format
      as check_h_progress.)

      TODO: Logically EvaluationContext should be "const" here, but we
      don't currently have a good const interface for
      EvaluationContext.
    */
    void set_initial_h_values(EvaluationContext &eval_context,
                              const SearchStatistics &statistics);

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
    bool check_h_progress(EvaluationContext &eval_context, int g,
                          const SearchStatistics &statistics);

    // output
    void print_initial_h_values() const;
};

#endif
