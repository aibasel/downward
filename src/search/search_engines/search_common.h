#ifndef SEARCH_ENGINES_SEARCH_COMMON_H
#define SEARCH_ENGINES_SEARCH_COMMON_H

/*
  This module contains functions for creating open list factories used
  by the search engines.

  TODO: Think about where this code should ideally live. One possible
  ordering principle: avoid unnecessary plug-in dependencies.

  This code currently depends on multiple different open list
  implementations, so it would be good if it would not be a dependency
  of search engines that don't need all these open lists. Under this
  maxim, EHC should not depend on this file. A logical solution might
  be to move every creation function that only uses one particular
  open list type into the header for this open list type, and leave
  this file for cases that require more complex set-up and are common
  to eager and lazy search.
*/

#include <memory>

class Evaluator;
class OpenListFactory;

namespace options {
class Options;
}

namespace search_common {
/*
  Create a standard scalar open list factory with the given "eval" and
  "pref_only" options.
*/
extern std::shared_ptr<OpenListFactory> create_standard_scalar_open_list_factory(
    const std::shared_ptr<Evaluator> &eval, bool pref_only);

/*
  Create open list factory for the eager_greedy or lazy_greedy plugins.

  Uses "evals", "preferred" and "boost" from the passed-in Options
  object to construct an open list factory of the appropriate type.

  This is usually an alternation open list with:
  - one sublist for each evaluator, considering all successors
  - one sublist for each evaluator, considering only preferred successors

  However, the preferred-only open lists are omitted if no preferred
  operator evaluators are used, and if there would only be one sublist
  for the alternation open list, then that sublist is returned
  directly.
*/
extern std::shared_ptr<OpenListFactory> create_greedy_open_list_factory(
    const options::Options &opts);

/*
  Create open list factory for the lazy_wastar plugin.

  Uses "evals", "preferred", "boost" and "w" from the passed-in
  Options object to construct an open list factory of the appropriate
  type.

  This works essentially the same way as parse_greedy (see
  documentation there), except that the open lists use evalators based
  on g + w * h rather than using h directly.
*/
extern std::shared_ptr<OpenListFactory> create_wastar_open_list_factory(
    const options::Options &opts);

/*
  Create open list factory and f_evaluator (used for displaying progress
  statistics) for A* search.

  The resulting open list factory produces a tie-breaking open list
  ordered primarily on g + h and secondarily on h. Uses "eval" from
  the passed-in Options object as the h evaluator.
*/
extern std::pair<std::shared_ptr<OpenListFactory>, const std::shared_ptr<Evaluator>>
create_astar_open_list_factory_and_f_eval(const options::Options &opts);
}

#endif
