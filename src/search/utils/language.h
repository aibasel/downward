#ifndef UTILS_LANGUAGE_H
#define UTILS_LANGUAGE_H

// TODO: this should depend on the compiler, not on the OS.
#if defined(_WIN32)
#define NO_RETURN __declspec(noreturn)
#else
#define NO_RETURN __attribute__((noreturn))
#endif

namespace Utils {
template<typename T>
void unused_parameter(const T &) {
}
}

#endif
