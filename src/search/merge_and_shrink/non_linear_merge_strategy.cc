#include "non_linear_merge_strategy.h"

#include "../option_parser.h"
#include "../plugin.h"

#include <cstdlib>
#include <iostream>

using namespace std;

NonLinearMergeStrategy::NonLinearMergeStrategy(const Options &opts)
    : MergeStrategy() {
    cout << opts.get<int>("test") << endl;
    exit(1);
}

bool NonLinearMergeStrategy::done() const {
    exit(1);
    return true;
}

std::pair<int, int> NonLinearMergeStrategy::get_next(const std::vector<Abstraction *> &all_abstractions) {
    exit(1);
    return make_pair(all_abstractions.size(), all_abstractions.size());
}

void NonLinearMergeStrategy::dump_strategy_specific_options() const {
    cout << "TODO" << endl;
}

string NonLinearMergeStrategy::name() const {
    return "non linear";
}
