#ifndef UTILS_LANGUAGE_H
#define UTILS_LANGUAGE_H

#if defined(_MSC_VER)
#define NO_RETURN __declspec(noreturn)
#else
#define NO_RETURN __attribute__((noreturn))
#endif

namespace utils {
template<typename T>
void unused_variable(const T &) {
}
}

#endif
