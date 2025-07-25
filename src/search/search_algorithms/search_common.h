#ifndef SEARCH_ALGORITHMS_SEARCH_COMMON_H
#define SEARCH_ALGORITHMS_SEARCH_COMMON_H

/*
  This module contains functions for creating open list factories used
  by the search algorithms.

  TODO: Think about where this code should ideally live. One possible
  ordering principle: avoid unnecessary plug-in dependencies.

  This code currently depends on multiple different open list
  implementations, so it would be good if it would not be a dependency
  of search algorithms that don't need all these open lists. Under this
  maxim, EHC should not depend on this file. A logical solution might
  be to move every creation function that only uses one particular
  open list type into the header for this open list type, and leave
  this file for cases that require more complex set-up and are common
  to eager and lazy search.
*/

#include <memory>
#include <vector>
#include "../utils/logging.h"

class Evaluator;
class TaskIndependentEvaluator;
class OpenListFactory;
class TaskIndependentOpenListFactory;

namespace search_common {
//issue559///*
//issue559//  Create open list factory for the eager_greedy or lazy_greedy plugins.
//issue559//
//issue559//  This is usually an alternation open list with:
//issue559//  - one sublist for each evaluator, considering all successors
//issue559//  - one sublist for each evaluator, considering only preferred successors
//issue559//
//issue559//  However, the preferred-only open lists are omitted if no preferred
//issue559//  operator evaluators are used, and if there would only be one sublist
//issue559//  for the alternation open list, then that sublist is returned
//issue559//  directly.
//issue559//*/
//issue559//extern std::shared_ptr<OpenListFactory> create_greedy_open_list_factory(
//issue559//    const std::vector<std::shared_ptr<Evaluator>> &evals,
//issue559//    const std::vector<std::shared_ptr<Evaluator>> &preferred_evaluators,
//issue559//    int boost);
//issue559//
//issue559///*
//issue559//  Create open list factory for the lazy_wastar plugin.
//issue559//
//issue559//  This works essentially the same way as parse_greedy (see
//issue559//  documentation there), except that the open lists use evalators based
//issue559//  on g + w * h rather than using h directly.
//issue559//*/
//issue559//extern std::shared_ptr<OpenListFactory> create_wastar_open_list_factory(
//issue559//    const std::vector<std::shared_ptr<Evaluator>> &base_evals,
//issue559//    const std::vector<std::shared_ptr<Evaluator>> &preferred, int boost,
//issue559//    int weight, utils::Verbosity verbosity);

/*
  Create open list factory and f_evaluator (used for displaying progress
  statistics) for A* search.

  The resulting open list factory produces a tie-breaking open list
  ordered primarily on g + h and secondarily on h.
*/
extern std::pair<std::shared_ptr<TaskIndependentOpenListFactory>,
                 const std::shared_ptr<TaskIndependentEvaluator>>
create_task_independent_astar_open_list_factory_and_f_eval(
    const std::shared_ptr<TaskIndependentEvaluator> &h_eval,
    const std::string &description,
    utils::Verbosity verbosity);
}

#endif
