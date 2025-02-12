#include "component_errors.h"

namespace utils {
void verify_argument(bool b, const std::string &message) {
    if (!b) {
        throw ComponentArgumentError(message);
    }
}
}
