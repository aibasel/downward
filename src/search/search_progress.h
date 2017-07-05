#ifndef SEARCH_PROGRESS_H
#define SEARCH_PROGRESS_H

#include <unordered_map>

class EvaluationContext;
class Heuristic;

/*
  This class helps track search progress.

  It maintains a record of best heuristic values and can determine if
  an evaluated state has a better heuristic value in at least one of
  the heuristics than all previously seen ones.
*/


class SearchProgress {
    std::unordered_map<const Heuristic *, int> best_heuristic_values;

    bool process_heuristic_value(const Heuristic *heuristic, int h);

public:
    SearchProgress() = default;
    ~SearchProgress() = default;

    /*
      Call the following function after each state evaluation.

      It keeps track of the best heuristic value for each heuristic
      evaluated, returning true if at least one heuristic value is the
      best value seen for this heuristic so far. (This includes the
      case where the evaluation context includes a heuristic that has
      not been evaluated previously, e.g. after evaluating the initial
      state.)

      Prints one line of output for each new best heuristic value.
    */
    bool check_progress(const EvaluationContext &eval_context);
};

#endif
