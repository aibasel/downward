#include "cegar_heuristic.h"

#include "../option_parser.h"
#include "../plugin.h"
#include "../state.h"
#include "../timer.h"
#include "../utilities.h"

#include <algorithm>
#include <cassert>
#include <set>
#include <vector>

using namespace std;

namespace cegar_heuristic {

CegarHeuristic::CegarHeuristic(const Options &opts)
    : Heuristic(opts),
      refinements(opts.get<int>("refinements")) {
}

CegarHeuristic::~CegarHeuristic() {
}

void CegarHeuristic::initialize() {
    cout << "Peak memory before refining: " << get_peak_memory_in_kb() << " KB" << endl;
    cout << "Initializing cegar heuristic..." << endl;
    abstraction = Abstraction();
    bool success = false;
    for (int i = 0; i < refinements; ++i) {
        if (i % 100 == 0)
            cout << "Refinement " << i << "/" << refinements << endl;
        bool solution_found = abstraction.find_solution();
        assert(solution_found);
        if (DEBUG)
            cout << "SOLUTION: " << abstraction.get_solution_string() << endl;
        success = abstraction.check_solution();
        if (DEBUG)
            cout << "NEXT ARCS: " << abstraction.get_init()->get_next_as_string()
                 << endl;
        if (success)
            break;
    }
    cout << "Done refining [t=" << g_timer << "]" << endl;
    cout << "Peak memory after refining: " << get_peak_memory_in_kb() << " KB" << endl;
    cout << "Solution found while refining: " << success << endl;
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
