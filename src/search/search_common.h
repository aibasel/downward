#ifndef SEARCH_COMMON_H
#define SEARCH_COMMON_H

/*
  This module contains functions for createn open list factories used
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

class OpenListFactory;
class Options;
class ScalarEvaluator;


/*
  Create a standard scalar open list factory with the given "eval" and
  "pref_only" options.
*/

extern std::shared_ptr<OpenListFactory> create_standard_scalar_open_list_factory(
    ScalarEvaluator *eval, bool pref_only);

/*
  Create open list factory for the eager_greedy or lazy_greedy plugins.

  Uses "evals", "preferred" and "boost" from the passed-in Options
  object to construct an open list factory of the appropriate type.

  This is usually an alternation open list with:
  - one sublist for each heuristic, considering all successors
  - one sublist for each heuristic, considering only preferred successors

  However, the preferred-only open lists are omitted if no preferred
  operator heuristics are used, and if there would only be one sublist
  for the alternation open list, then that sublist is returned
  directly.
*/

extern std::shared_ptr<OpenListFactory> create_greedy_open_list_factory(
    const Options &opts);

#endif
