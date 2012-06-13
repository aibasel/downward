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
}

CegarHeuristic::~CegarHeuristic() {
}

void CegarHeuristic::initialize() {
    cout << "Initializing cegar heuristic..." << endl;
    abstraction = Abstraction();
    for (int i = 0; i < REFINEMENTS; ++i) {
        bool solution_found = abstraction.find_solution();
        assert(solution_found);
        cout << "SOLUTION: " << abstraction.get_solution_string() << endl;
        bool success = abstraction.check_solution();
        cout << "SUCCESS: " << success << endl;
        if (success)
            break;
    }
    abstraction.calculate_costs();
}

int CegarHeuristic::compute_heuristic(const State &state) {
    return abstraction.get_abstract_state(state)->get_distance();
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
