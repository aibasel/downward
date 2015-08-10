#include "util.h"

#include "potential_function.h"
#include "potential_optimizer.h"

#include "../countdown_timer.h"
#include "../option_parser.h"
#include "../sampling.h"
#include "../successor_generator.h"

#include <limits>

using namespace std;


namespace potentials {
vector<State> sample_without_dead_end_detection(
    PotentialOptimizer &optimizer, int num_samples) {
    const shared_ptr<AbstractTask> task = optimizer.get_task();
    const TaskProxy task_proxy(*task);
    State initial_state = task_proxy.get_initial_state();
    optimizer.optimize_for_state(initial_state);
    SuccessorGenerator successor_generator(task);
    int init_h = optimizer.get_potential_function()->get_value(initial_state);
    CountdownTimer timer(numeric_limits<double>::infinity());
    return sample_states_with_random_walks(
               task_proxy, successor_generator, num_samples, init_h,
               get_average_operator_cost(task_proxy),
               [] (const State &) {
                   // Currently, our potential functions can't detect dead ends.
                   return false;
               },
               timer);
}

void add_common_potentials_options_to_parser(OptionParser &parser) {
    parser.document_language_support("action costs", "supported");
    parser.document_language_support("conditional effects", "not supported");
    parser.document_language_support("axioms", "not supported");
    parser.document_property("admissible", "yes");
    parser.document_property("consistent", "yes");
    parser.document_property("safe", "yes");
    parser.document_property("preferred operators", "no");
    parser.add_option<double>(
        "max_potential",
        "Bound potentials by this number",
        "1e8",
        Bounds("0.0", "infinity"));
    add_lp_solver_option_to_parser(parser);
    Heuristic::add_options_to_parser(parser);
}
}
