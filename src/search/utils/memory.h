#ifndef UTILS_MEMORY_H
#define UTILS_MEMORY_H

#include <memory>
#include <utility>

namespace utils {
/*
  make_unique_ptr is a poor man's version of make_unique. Once we
  require C++14, we should change all occurrences of make_unique_ptr
  to make_unique.
*/
template<typename T, typename ... Args>
std::unique_ptr<T> make_unique_ptr(Args && ... args) {
    return std::unique_ptr<T>(new T(std::forward<Args>(args) ...));
}

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
