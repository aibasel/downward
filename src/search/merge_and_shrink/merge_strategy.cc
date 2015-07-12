#include "merge_strategy.h"

#include "../task_proxy.h"

#include <cassert>
#include <iostream>

using namespace std;

MergeStrategy::MergeStrategy() : remaining_merges(-1) {
}

void MergeStrategy::initialize(shared_ptr<AbstractTask> task) {
    assert(!initialized());
    /*
      There are number of variables many atomic transition systems and we have
      to perform one less merges than this number until we have merged
      all transition systems into one composite transition system.
    */
    TaskProxy task_proxy(*task);
    remaining_merges = task_proxy.get_variables().size() - 1;
}

bool MergeStrategy::initialized() const {
    return remaining_merges != -1;
}

bool MergeStrategy::done() const {
    assert(initialized());
    return remaining_merges == 0;
}

void MergeStrategy::dump_options() const {
    cout << "Merge strategy options:" << endl;
    cout << "Type: " << name() << endl;
    dump_strategy_specific_options();
}
