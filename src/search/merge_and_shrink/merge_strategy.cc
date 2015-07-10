#include "merge_strategy.h"

#include "../task_proxy.h"

#include <iostream>

using namespace std;

MergeStrategy::MergeStrategy() {
}

void MergeStrategy::initialize(const TaskProxy &task_proxy) {
    /*
      There are number of variables many atomic transition systems and we have
      to perform one less merges than this number until we have merged
      all transition systems into one composite transition system.
    */
    remaining_merges = task_proxy.get_variables().size() - 1;
}

void MergeStrategy::dump_options() const {
    cout << "Merge strategy options:" << endl;
    cout << "Type: " << name() << endl;
    dump_strategy_specific_options();
}
