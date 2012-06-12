#include "./cegar_heuristic.h"

#include "./../globals.h"
#include "./../operator.h"
#include "./../option_parser.h"
#include "./../plugin.h"
#include "./../state.h"

#include <assert.h>

#include <limits>
#include <utility>
#include <iostream>
#include <algorithm>
#include <set>
#include <vector>
using namespace std;

namespace cegar_heuristic {

int REFINEMENTS = 2;

CegarHeuristic::CegarHeuristic(const Options &opts)
    : Heuristic(opts) {
    min_operator_cost = numeric_limits<int>::max();
    for (int i = 0; i < g_operators.size(); ++i)
        min_operator_cost = min(min_operator_cost,
                                get_adjusted_cost(g_operators[i]));
}

CegarHeuristic::~CegarHeuristic() {
}

void CegarHeuristic::initialize() {
    cout << "Initializing cegar heuristic..." << endl;
    abstraction = Abstraction();
    for (int i = 0; i < REFINEMENTS; ++i) {
        abstraction.find_solution();
        cout << abstraction.get_solution_string() << endl;
        bool success = abstraction.check_solution();
        cout << "SUCCESS: " << success << endl;
        if (success)
            break;
    }
}

int CegarHeuristic::compute_heuristic(const State &state) {
    if (test_goal(state))
        return 0;
    else
        return min_operator_cost;
}

static ScalarEvaluator *_parse(OptionParser &parser) {
    Heuristic::add_options_to_parser(parser);
    Options opts = parser.parse();
    if (parser.dry_run())
        return 0;
    else
        return new CegarHeuristic(opts);
}

static Plugin<ScalarEvaluator> _plugin("cegar", _parse);

}
