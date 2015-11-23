#include "lm_cut_heuristic.h"

#include "lm_cut_landmarks.h"

#include "../option_parser.h"
#include "../plugin.h"
#include "../task_proxy.h"
#include "../task_tools.h"

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
    landmark_generator = make_unique_ptr<LandmarkCutLandmarks>(task_proxy);
}

int LandmarkCutHeuristic::compute_heuristic(const GlobalState &global_state) {
    State state = convert_global_state(global_state);
    return compute_heuristic(state);
}

int LandmarkCutHeuristic::compute_heuristic(const State &state) {
    int total_cost = 0;
    bool dead_end = landmark_generator->compute_landmarks(
        state,
        [&total_cost](int cut_cost) {total_cost += cut_cost; },
        nullptr);

    if (dead_end)
        return DEAD_END;
    return total_cost;
}

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
        return nullptr;
    else
        return new LandmarkCutHeuristic(opts);
}


static Plugin<Heuristic> _plugin("lmcut", _parse);
