#include "merge_strategy.h"

using namespace std;

namespace merge_and_shrink {
MergeStrategy::MergeStrategy(
    FactoredTransitionSystem &fts)
    : fts(fts) {
}
}
