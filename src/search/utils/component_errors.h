#ifndef UTILS_COMPONENT_ERRORS_H
#define UTILS_COMPONENT_ERRORS_H

#include "exceptions.h"

#include <string>
#include <vector>

namespace utils {
class ComponentArgumentError : public Exception {
public:
    explicit ComponentArgumentError(const std::string &msg) : Exception(msg) {}
};

void verify_argument(bool b, const std::string &message);

template<typename T>
void verify_list_not_empty(const std::vector<T> list, const std::string &name) {
    if (list.empty()) {
        throw ComponentArgumentError("List argument '" + name + "' has to be non-empty.");
    }
}
}
#endif
