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
    std::unordered_map<const Heuristic *, int> initial_heuristic_values;
    std::unordered_map<const Heuristic *, int> best_heuristic_values;

    bool process_heuristic_value(const Heuristic *heuristic, int h);
    void output_line(const std::string &description,
                     const Heuristic *heuristic, int h) const;

public:
    SearchProgress() = default;
    ~SearchProgress() = default;

    /*
      Call the following function after each state evaluation.

      It keeps track of the initial and best heuristic value for each
      heuristic evaluated, returning true if at least one heuristic
      value is the best value seen for this heuristic so far. (This
      includes the case where the evaluation context includes a
      heuristic that has not been evaluated previously, e.g. after
      evaluating the initial state.)

      Prints one line of output for each new best heuristic value.
    */
    bool check_progress(const EvaluationContext &eval_context);

    /*
      Print information on the first heuristic value seen for each heuristic.

      (This assumes that check_progress has previously been called to
      feed information to the SearchProgress instance.)
    */
    void print_initial_h_values() const;
};

#endif
