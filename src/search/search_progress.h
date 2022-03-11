#ifndef SEARCH_PROGRESS_H
#define SEARCH_PROGRESS_H

#include <unordered_map>

class EvaluationContext;
class Evaluator;

namespace utils {
class LogProxy;
}

/*
  This class helps track search progress.

  Evaluators can be configured to be used for reporting new minima, boosting
  open lists, or both. This class maintains a record of minimum evaluator
  values for evaluators that are used for either of these two things.
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
      for at least one evaluator used for boosting.

      It also prints one line of output for all evaluators used for reporting
      minima that have a new minimum value in the given evaluation context.

      In both cases this includes the situation where the evaluator in question
      has not been evaluated previously, e.g., after evaluating the initial
      state.
    */
    bool check_progress(const EvaluationContext &eval_context);
};

#endif
