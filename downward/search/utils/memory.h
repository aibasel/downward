#ifndef UTILS_MEMORY_H
#define UTILS_MEMORY_H

namespace utils {
/*
  Reserve some memory that we can release and be able to continue
  afterwards, once we hit the memory limit. Due to memory fragmentation
  the planner often doesn't have enough memory to continue if we don't
  reserve enough memory. For CEGAR heuristics reserving 75 MB worked
  best.

  The interface assumes a single user. It is not possible for two parts
  of the planner to reserve extra memory padding at the same time.
*/
extern void reserve_extra_memory_padding(int memory_in_mb);
extern void release_extra_memory_padding();
extern bool extra_memory_padding_is_reserved();
}

#endif
