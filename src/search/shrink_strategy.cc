#include <cassert>
#include <iostream>
#include <vector>
#include "raz_abstraction.h"
#include "shrink_strategy.h"

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
        cout << "shrink by " << (abs.size() - threshold) << " nodes" << " (from "
             << abs.size() << " to " << threshold << ")" << endl;
    else if (force)
        cout << "shrink forced: prune unreachable/irrelevant states" << endl;
    else if (is_bisimulation())
        cout << "shrink due to bisimulation strategy" << endl;
    else
        return false;
    return true;
}

void ShrinkStrategy::apply(
    Abstraction &abs, 
    vector<slist<AbstractStateRef> > &collapsed_groups, 
    int threshold) const {
    assert(collapsed_groups.size() <= threshold);
    abs.apply_abstraction(collapsed_groups);
    cout << "size of abstraction after shrink: " << abs.size()
         << ", Threshold: " << threshold << endl;
    assert(abs.size() <= threshold || threshold == 1);
}
