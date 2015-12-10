#ifndef UTILS_MEMORY_H
#define UTILS_MEMORY_H

#include <memory>
#include <utility>


namespace Utils {
/*
  make_unique_ptr is a poor man's version of make_unique. Once we
  require C++14, we should change all occurrences of make_unique_ptr
  to make_unique.
*/
template<typename T, typename ... Args>
std::unique_ptr<T> make_unique_ptr(Args && ... args) {
    return std::unique_ptr<T>(new T(std::forward<Args>(args) ...));
}
}

#endif
