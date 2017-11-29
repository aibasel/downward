#ifndef SEARCH_PROGRESS_H
#define SEARCH_PROGRESS_H

#include <unordered_map>

class EvaluationContext;
class Evaluator;

/*
  This class helps track search progress.

  It maintains a record of minimum evaluator values and can determine if an
  evaluated state has a lower value for at least one of the evaluators than all
  previously seen ones. It only tracks evaluators for which enable_statistics
  is set to true. This usually only includes heuristics.
*/


class SearchProgress {
    std::unordered_map<const Evaluator *, int> min_values;

    bool process_value(const Evaluator *evaluator, int value);

public:
    SearchProgress() = default;
    ~SearchProgress() = default;

    /*
      Call the following function after each state evaluation.

      It keeps track of the minimum result for each evaluator, returning true
      if at least one value is the lowest value seen for this evaluator so far.
      (This includes the case where the evaluation context includes an
      evaluator that has not been evaluated previously, e.g., after evaluating
      the initial state.)

      Prints one line of output for each new minimum evaluator value.
    */
    bool check_progress(const EvaluationContext &eval_context);
};

#endif
