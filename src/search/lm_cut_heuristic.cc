#include "lm_cut_heuristic.h"

#include "lm_cut_landmarks.h"
#include "option_parser.h"
#include "plugin.h"
#include "task_proxy.h"
#include "task_tools.h"

#include <iostream>

using namespace std;


// construction and destruction
LandmarkCutHeuristic::LandmarkCutHeuristic(const Options &opts)
    : Heuristic(opts),
    landmark_generator(nullptr) {
}

LandmarkCutHeuristic::~LandmarkCutHeuristic() {
}

// initialization
void LandmarkCutHeuristic::initialize() {
    cout << "Initializing landmark cut heuristic..." << endl;
    // TODO we don't need a pointer if we initialize in the constructor.
    landmark_generator = make_unique_ptr<LandmarkCutLandmarks>(task);
}

int LandmarkCutHeuristic::compute_heuristic(const GlobalState &global_state) {
    State state = convert_global_state(global_state);

    int total_cost = 0;

    bool dead_end = landmark_generator->compute_landmarks(
        state,
        [&total_cost](int cut_cost) {
            total_cost += cut_cost;
        },
        nullptr
    );

    if (dead_end)
        return DEAD_END;
    return (total_cost + COST_MULTIPLIER - 1) / COST_MULTIPLIER;
}

/* TODO:
   It looks like the change in r3638 reduced the quality of the heuristic
   a bit (at least a preliminary glance at Elevators-03 suggests that).
   The only arbitrary aspect is the tie-breaking policy in choosing h_max
   supporters, so maybe that is adversely affected by the incremental
   procedure? In that case, maybe we should play around with this a bit,
   e.g. use a different tie-breaking rule in every round to spread out the
   values a bit.
 */

static Heuristic *_parse(OptionParser &parser) {
    parser.document_synopsis("Landmark-cut heuristic", "");
    parser.document_language_support("action costs", "supported");
    parser.document_language_support("conditional effects", "not supported");
    parser.document_language_support("axioms", "not supported");
    parser.document_property("admissible", "yes");
    parser.document_property("consistent", "no");
    parser.document_property("safe", "yes");
    parser.document_property("preferred operators", "no");

    Heuristic::add_options_to_parser(parser);
    Options opts = parser.parse();
    if (parser.dry_run())
        return 0;
    else
        return new LandmarkCutHeuristic(opts);
}


static Plugin<Heuristic> _plugin("lmcut", _parse);
