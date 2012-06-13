#include "./cegar_heuristic.h"

#include <assert.h>

#include <algorithm>
#include <set>
#include <vector>

#include "./../globals.h"
#include "./../option_parser.h"
#include "./../plugin.h"
#include "./../state.h"

using namespace std;

namespace cegar_heuristic {

CegarHeuristic::CegarHeuristic(const Options &opts)
    : Heuristic(opts),
      refinements(opts.get<int>("refinements")) {
}

CegarHeuristic::~CegarHeuristic() {
}

void CegarHeuristic::initialize() {
    cout << "Initializing cegar heuristic..." << endl;
    abstraction = Abstraction();
    for (int i = 0; i < refinements; ++i) {
        bool solution_found = abstraction.find_solution();
        assert(solution_found);
        cout << "SOLUTION: " << abstraction.get_solution_string() << endl;
        bool success = abstraction.check_solution();
        if (DEBUG)
            cout << "NEXT ARCS: " << abstraction.get_init()->get_next_as_string()
                 << endl;
        if (success)
            break;
    }
    abstraction.calculate_costs();
}

int CegarHeuristic::compute_heuristic(const State &state) {
    return abstraction.get_abstract_state(state)->get_distance();
}

static ScalarEvaluator *_parse(OptionParser &parser) {
    parser.add_option<int>("refinements", 100, "number of refinements");
    Heuristic::add_options_to_parser(parser);
    Options opts = parser.parse();
    if (parser.dry_run())
        return 0;
    else
        return new CegarHeuristic(opts);
}

static Plugin<ScalarEvaluator> _plugin("cegar", _parse);
}
