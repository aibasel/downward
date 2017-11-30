#ifndef SEARCH_PROGRESS_H
#define SEARCH_PROGRESS_H

#include <unordered_map>

class EvaluationContext;
class Evaluator;

/*
  This class helps track search progress.

  Evaluators can be configured to be used for reporting new minima, boosting
  open lists, or both. We call evaluators that are used for either of these two
  things "tracked".

  This class maintains a record of minimum evaluator values for tracked
  evaluators and can determine if an evaluated state has a lower value for at
  least one of the tracked evaluators than all previously seen ones.
*/


class SearchProgress {
    std::unordered_map<const Evaluator *, int> min_values;

    bool process_evaluator_value(const Evaluator *evaluator, int value);

public:
    SearchProgress() = default;
    ~SearchProgress() = default;

    /*
      Call the following function after each state evaluation.

      It returns true if the evaluation context contains a new minimum value
      for at least one tracked evaluator. (This includes the case where the
      evaluation context includes an evaluator that has not been evaluated
      previously, e.g., after evaluating the initial state.)

      Prints one line of output for each new minimum evaluator value.
    */
    bool check_progress(const EvaluationContext &eval_context);
};

#endif
