#ifndef UTILS_COMPONENT_ERRORS_H
#define UTILS_COMPONENT_ERRORS_H

#include "exceptions.h"

#include <vector>

namespace utils {
class ComponentArgumentError : public Exception {
public:
    explicit ComponentArgumentError(const std::string &msg) : Exception(msg) {}
};

template <typename T>
void verify_list_not_empty(const std::vector<T> list, const std::string &name) {
    if (list.empty()) {
        throw ComponentArgumentError("List argument '" + name + "' has to be non-empty.");
    }
}

template <typename T, typename F>
void verify_comparison(const T arg1, const T arg2, F comparator, const std::string &message) {
    if (!comparator(arg1, arg2)){
        throw ComponentArgumentError(message);
    }
}
}
#endif
