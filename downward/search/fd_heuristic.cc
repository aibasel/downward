#include "abstraction.h"
#include "fd_heuristic.h"
#include "globals.h"
#include "operator.h"
#include "option_parser.h"
#include "state.h"
#include "timer.h"
#include "variable_order_finder.h"

#include <cassert>
#include <cmath>
#include <vector>
using namespace std;


FinkbeinerDraegerHeuristic::FinkbeinerDraegerHeuristic() {
}

FinkbeinerDraegerHeuristic::~FinkbeinerDraegerHeuristic() {
}

void FinkbeinerDraegerHeuristic::verify_no_axioms_no_cond_effects() const {
    if(!g_axioms.empty()) {
        cerr << "Heuristic does not support axioms!" << endl
             << "Terminating." << endl;
        exit(1);
    }
    if (g_use_metric) {
    	cerr << "Warning: M&S heuristic does not support action costs!" << endl;
    	if (g_min_action_cost == 0) {
    		cerr << "Alert: 0-cost actions exist. M&S Heuristic is not admissible" << endl;
    	}
    }


    for(int i = 0; i < g_operators.size(); i++) {
        const vector<PrePost> &pre_post = g_operators[i].get_pre_post();
        for(int j = 0; j < pre_post.size(); j++) {
            const vector<Prevail> &cond = pre_post[j].cond;
            if(cond.empty())
                continue;
            // Accept conditions that are redundant, but nothing else.
            // In a better world, these would never be included in the
            // input in the first place.
            int var = pre_post[j].var;
            int pre = pre_post[j].pre;
            int post = pre_post[j].post;
            if(pre == -1 && cond.size() == 1 &&
               cond[0].var == var && cond[0].prev != post &&
               g_variable_domain[var] == 2)
                continue;

            cerr << "Heuristic does not support conditional effects "
                 << "(operator " << g_operators[i].get_name() << ")"
                 << endl << "Terminating." << endl;
            exit(1);
        }
    }
}

Abstraction *FinkbeinerDraegerHeuristic::build_abstraction(bool is_first) {
    cout << "Merging abstractions..." << endl;
    assert(!g_abstractions.empty());
    int threshold = g_abstraction_max_size;

    VariableOrderFinder order(is_first);

    Abstraction *abstraction = g_abstractions[order.next()];
    abstraction->statistics();

    bool first_iteration = true;
    while(!order.done() && abstraction->is_solvable()) {
        int var_no = order.next();

        int max_allowed_size;
        int atomic_abstraction_target_size = g_abstractions[var_no]->size();
        if(g_merge_and_shrink_bound_is_for_product) {
            int balanced_size = int(sqrt(threshold));
            if(atomic_abstraction_target_size > balanced_size)
                atomic_abstraction_target_size = balanced_size;
            max_allowed_size = threshold / atomic_abstraction_target_size;
        } else {
            max_allowed_size = threshold;
            if(atomic_abstraction_target_size > threshold)
                atomic_abstraction_target_size = threshold;
        }

        if(atomic_abstraction_target_size != g_abstractions[var_no]->size()) {
            cout << "atomic abstraction too big; must shrink" << endl;
            g_abstractions[var_no]->shrink(atomic_abstraction_target_size);
        }

        if(abstraction->size() > max_allowed_size) {
            abstraction->shrink(max_allowed_size);
            abstraction->statistics();
        }
        Abstraction *new_abstraction = new CompositeAbstraction(
            abstraction, g_abstractions[var_no]);
        if(first_iteration)
            first_iteration = false;
        else
            abstraction->release_memory();
        abstraction = new_abstraction;
        abstraction->statistics();
    }
    return abstraction;
}

void FinkbeinerDraegerHeuristic::initialize() {
    int threshold = g_abstraction_max_size;
    Timer timer;
    cout << "Initializing Finkbeiner/Draeger heuristic..." << endl;
    verify_no_axioms_no_cond_effects();
    cout << "Abstraction size limit: " << threshold << endl;

    cout << "Building initial abstractions..." << endl;
    Abstraction::build_initial_abstractions(g_abstractions);

    for(int i = 0; i < g_abstraction_nr; i++) {
        cout << "Building abstraction nr " << i << "..." << endl;
        abstractions.push_back(build_abstraction(i == 0));
        if(!abstractions.back()->is_solvable())
            break;
    }

    cout << "Done initializing Finkbeiner/Draeger heuristic ["
         << timer << "]" << endl
         << "initial h value: " << compute_heuristic(*g_initial_state)
         << endl;
    cout << "Peak abstraction size: "
         << g_abstraction_peak_memory << " bytes" << endl;


}

int FinkbeinerDraegerHeuristic::compute_heuristic(const State &state) {
    int cost = 0;
    for(int i = 0; i < abstractions.size(); i++) {
        int abs_cost = abstractions[i]->get_cost(state);
        if(abs_cost == -1)
            return DEAD_END;
        cost = max(cost, abs_cost);
    }
    if(cost == 0) {
        /* We don't want to report 0 for non-goal states because the
           search code doesn't like that. Note that we might report 0
           for non-goal states if we use tiny abstraction sizes (like
           1) or random shrinking. */
        // TODO: Change this once we support action costs!
        for(int i = 0; i < g_goal.size(); i++) {
            int var = g_goal[i].first, value = g_goal[i].second;
            if(state[var] != value) {
                cost = 1;
                break;
            }
        }
    }
    return cost;
}

ScalarEvaluator *FinkbeinerDraegerHeuristic::create(
    const std::vector<string> &config, int start, int &end) {
    if (g_using_abstraction_heuristic) {
        cerr << "The current implementation supports only one "
             << "abstraction heuristic" << endl;
        exit(2);
    }
    
    // "<name>()" or "<name>(<options>)"
    if (config.size() > start + 2 && config[start + 1] == "(") {
        end = start + 2;

        // TODO: better documentation what each parameter does
        if (config[end] != ")") { 
            NamedOptionParser option_parser;
            option_parser.add_int_option("max_states",
                                         &g_abstraction_max_size, 
                                         "maximum abstraction size");
            option_parser.add_int_option("count",
                                         &g_abstraction_nr, 
                                         "nr of abstractions to build");
            option_parser.add_int_option("merge_strategy",
                                         &g_compose_strategy, 
                                         "merge strategy");
            option_parser.add_int_option("shrink_strategy",
                                         &g_collapse_strategy, 
                                         "shrink strategy");
            option_parser.add_bool_option("bound_is_for_product",
                                         &g_merge_and_shrink_bound_is_for_product,
                                         "merge and shrink bound is for product");
            option_parser.parse_options(config, end, end);
            end ++;
        }
        if (config[end] != ")") throw ParseError(end);
        
    } else { // "<name>"
        end = start;
    }

    if (g_abstraction_max_size < 1) {
        cerr << "error: abstraction size must be at least 1"
             << endl;
        exit(2);
    }
                

    cout << "Composition strategy: ";
    switch (g_compose_strategy) {
        case COMPOSE_LINEAR_CG_GOAL_LEVEL:
            cout << "linear CG/GOAL, tie breaking on level (main)"; break;
        case COMPOSE_LINEAR_CG_GOAL_RANDOM:
            cout << "linear CG/GOAL, tie breaking random"; break;
        case COMPOSE_LINEAR_GOAL_CG_LEVEL:
            cout << "linear GOAL/CG, tie breaking on level"; break;
        case COMPOSE_LINEAR_RANDOM:
            cout << "linear random"; break;
        case COMPOSE_DFP:
            cout << "Draeger/Finkbeiner/Podelski" << endl;
            cerr << "DFP composition strategy not implemented." << endl;
            exit(2);
        default:
            if (g_compose_strategy < 0 ||
                g_compose_strategy >= MAX_COMPOSE_STRATEGY) {
                cerr << "Unknown merge strategy: " << g_compose_strategy << endl;
                exit(2);
            }
    }
    cout << endl;

    cout << "Collapsing strategy: ";
    switch (g_collapse_strategy) {
        case COLLAPSE_HIGH_F_LOW_H:
            cout << "high f/low h (main)"; break;
        case COLLAPSE_LOW_F_LOW_H:
            cout << "low f/low h"; break;
        case COLLAPSE_HIGH_F_HIGH_H:
            cout << "high f/high h"; break;
        case COLLAPSE_RANDOM:
            cout << "random states"; break;
        case COLLAPSE_DFP:
            cout << "Draeger/Finkbeiner/Podelski"; break;
        default:
            if (g_collapse_strategy < 0 ||
                g_collapse_strategy >= MAX_COLLAPSE_STRATEGY) {
                cerr << "Unknown shrink strategy: " << g_collapse_strategy << endl;
                exit(2);
            }
    }
    cout << endl;

    return new FinkbeinerDraegerHeuristic();
}
