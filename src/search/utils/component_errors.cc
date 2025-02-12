#include "component_errors.h"

using namespace std;

namespace utils {
void verify_argument(bool b, const string &message) {
    if (!b) {
        throw ComponentArgumentError(message);
    }
}
}
