#ifndef CEGAR_TYPES_H
#define CEGAR_TYPES_H

#include <limits>
#include <memory>
#include <unordered_set>
#include <utility>
#include <vector>

class AbstractTask;

namespace cegar {
class AbstractState;

// Transitions are pairs of operator index and AbstractState pointers.
using Arc = std::pair<int, AbstractState *>;
}

#endif
