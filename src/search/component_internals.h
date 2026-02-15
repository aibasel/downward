#ifndef COMPONENT_INTERNALS_H
#define COMPONENT_INTERNALS_H

#include "utils/hash.h"
#include "utils/language.h"
#include "utils/tuples.h"

#include <concepts>
#include <memory>
#include <vector>

class AbstractTask;
class TaskSpecificComponent;
class TaskIndependentComponentBase;

using CacheKey =
    std::pair<const TaskIndependentComponentBase *, const AbstractTask *>;
using Cache = utils::HashMap<CacheKey, std::shared_ptr<TaskSpecificComponent>>;
#endif
