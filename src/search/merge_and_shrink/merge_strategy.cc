#include "merge_strategy.h"

#include "../plugin.h"
#include "../task_proxy.h"

#include <cassert>
#include <iostream>

using namespace std;

namespace merge_and_shrink {
MergeStrategy::MergeStrategy(
    FactoredTransitionSystem &fts)
    : fts(fts) {
}
}
