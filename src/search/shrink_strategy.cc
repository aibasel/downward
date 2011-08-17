#include "shrink_strategy.h"

#include "raz_abstraction.h"

#include <cassert>
#include <iostream>
#include <vector>

using namespace std;
using namespace __gnu_cxx;


ShrinkStrategy::ShrinkStrategy() {
}

ShrinkStrategy::~ShrinkStrategy() {
}

bool ShrinkStrategy::must_shrink(
    const Abstraction &abs, int threshold, bool force) const {
    assert(threshold >= 1);
    assert(abs.is_solvable());
    if (abs.size() > threshold)
        cout << "shrink by " << (abs.size() - threshold) << " nodes"
             << " (from " << abs.size() << " to " << threshold << ")" << endl;
    else if (force)
        cout << "shrink forced: prune unreachable/irrelevant states" << endl;
    else if (is_bisimulation())
        cout << "shrink due to bisimulation strategy" << endl;
    else
        return false;
    return true;
}

/*
  TODO: I think we could get a nicer division of responsibilities if
  this method were part of the abstraction class. The shrink
  strategies would then return generate an equivalence class
  ("collapsed_groups") and not modify the abstraction, which would be
  passed as const.
 */

void ShrinkStrategy::apply(
    Abstraction &abs,
    EquivalenceRelation &equivalence_relation,
    int threshold) const {
    assert(equivalence_relation.size() <= threshold);
    abs.apply_abstraction(equivalence_relation);
    cout << "size of abstraction after shrink: " << abs.size()
         << ", threshold: " << threshold << endl;
    // TODO: Get rid of special-casing of threshold 1.
    assert(abs.size() <= threshold || threshold == 1);
}
