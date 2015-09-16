#include "util.h"

#include "potential_function.h"
#include "potential_optimizer.h"

#include "../option_parser.h"
#include "../sampling.h"
#include "../successor_generator.h"
#include "../task_tools.h"

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
    return sample_states_with_random_walks(
               task_proxy, successor_generator, num_samples, init_h,
               get_average_operator_cost(task_proxy));
}

string get_admissible_potentials_reference() {
    return "The algorithm is based on\n\n"
           " * Jendrik Seipp, Florian Pommerening and Malte Helmert.<<BR>>\n"
           " [New Optimization Functions for Potential Heuristics "
           "http://ai.cs.unibas.ch/papers/seipp-et-al-icaps2015.pdf]."
           "<<BR>>\n "
           "In //Proceedings of the 25th International Conference on "
           "Automated Planning and Scheduling (ICAPS 2015)//, pp. 193-201. "
           "AAAI Press 2015.\n\n\n";
}

void prepare_parser_for_admissible_potentials(OptionParser &parser) {
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
