#include "abstraction.h"
#include "fd_heuristic.h"
#include "globals.h"
#include "operator.h"
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
