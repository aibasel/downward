#include "merge_strategy.h"

#include <iostream>

using namespace std;

void MergeStrategy::dump_options() const {
    cout << "Merge strategy: " << name() << endl;
    dump_strategy_specific_options();
}
