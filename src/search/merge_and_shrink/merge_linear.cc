#include "merge_linear.h"

#include "../option_parser.h"
#include "../plugin.h"
#include "../utilities.h"

#include <cassert>
#include <cstdlib>

using namespace std;

MergeLinear::MergeLinear(const Options &opts)
    : MergeStrategy(),
      order(VariableOrderType(opts.get_enum("variable_order"))),
      need_first_index(true) {
}

bool MergeLinear::done() const {
    return order.done();
}

pair<int, int> MergeLinear::get_next(const std::vector<Abstraction *> &all_abstractions) {
    int first;
    if (need_first_index) {
        need_first_index = false;
        first = order.next();
        cout << "First variable: " << first << endl;
    } else {
        first = all_abstractions.size() - 1;
    }
    int second = order.next();
    cout << "Next variable: " << second << endl;
    assert(all_abstractions[first]);
    if (!all_abstractions[second]) {
        exit_with(EXIT_CRITICAL_ERROR);
    }
    assert(all_abstractions[second]);
    return make_pair(first, second);
}

void MergeLinear::dump_strategy_specific_options() const {
    cout << "Linear merge strategy: ";
    order.dump();
}

string MergeLinear::name() const {
    return "linear";
}

static MergeStrategy *_parse(OptionParser &parser) {
    vector<string> merge_strategies;
    //TODO: it's a bit dangerous that the merge strategies here
    // have to be specified exactly in the same order
    // as in the enum definition. Try to find a way around this,
    // or at least raise an error when the order is wrong.
    merge_strategies.push_back("CG_GOAL_LEVEL");
    merge_strategies.push_back("CG_GOAL_RANDOM");
    merge_strategies.push_back("GOAL_CG_LEVEL");
    merge_strategies.push_back("RANDOM");
    merge_strategies.push_back("LEVEL");
    merge_strategies.push_back("REVERSE_LEVEL");
    parser.add_enum_option("variable_order", merge_strategies,
                           "linear merge strategy",
                           "CG_GOAL_LEVEL");

    Options opts = parser.parse();
    if (parser.dry_run())
        return 0;
    else
        return new MergeLinear(opts);
}

static Plugin<MergeStrategy> _plugin("merge_linear", _parse);

