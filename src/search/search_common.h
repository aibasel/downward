#ifndef SEARCH_COMMON_H
#define SEARCH_COMMON_H

/*
  This module contains some common functionality used by eager and
  lazy search.
*/

#include <memory>

class OpenListFactory;
class Options;


/*
  Open list factory used internally by eager_greedy and lazy_greedy.
  This is not a user-visible plugin, but rather used internally by the
  eager_greedy and lazy_greedy shortcuts to create their open lists.

  TODO: Say more about the interface and what this does.
*/

extern std::shared_ptr<OpenListFactory> create_greedy_open_list_factory(
    const Options &opts);

#endif
