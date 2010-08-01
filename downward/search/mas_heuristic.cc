#include "mas_heuristic.h"

#include "abstraction.h"
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


MergeAndShrinkHeuristic::MergeAndShrinkHeuristic(
    int max_abstract_states_, bool bound_is_for_product_,
    int abstraction_count_,
    MergeStrategy merge_strategy_, ShrinkStrategy shrink_strategy_,
    bool use_label_simplification_, bool use_expensive_statistics_)
    : max_abstract_states(max_abstract_states_),
      bound_is_for_product(bound_is_for_product_),
      abstraction_count(abstraction_count_),
      merge_strategy(merge_strategy_),
      shrink_strategy(shrink_strategy_),
      use_label_simplification(use_label_simplification_),
      use_expensive_statistics(use_expensive_statistics_) {
    dump_options();
    warn_on_unusual_options();
}

MergeAndShrinkHeuristic::~MergeAndShrinkHeuristic() {
}

void MergeAndShrinkHeuristic::dump_options() const {
    cout << "Merge strategy: ";
    switch (merge_strategy) {
        case MERGE_LINEAR_CG_GOAL_LEVEL:
            cout << "linear CG/GOAL, tie breaking on level (main)"; break;
        case MERGE_LINEAR_CG_GOAL_RANDOM:
            cout << "linear CG/GOAL, tie breaking random"; break;
        case MERGE_LINEAR_GOAL_CG_LEVEL:
            cout << "linear GOAL/CG, tie breaking on level"; break;
        case MERGE_LINEAR_RANDOM:
            cout << "linear random"; break;
        case MERGE_DFP:
            cout << "Draeger/Finkbeiner/Podelski" << endl;
            cerr << "DFP merge strategy not implemented." << endl;
            exit(2);
        default:
            abort();
    }
    cout << endl;

    cout << "Shrink strategy: ";
    switch (shrink_strategy) {
        case SHRINK_HIGH_F_LOW_H:
            cout << "high f/low h (main)"; break;
        case SHRINK_LOW_F_LOW_H:
            cout << "low f/low h"; break;
        case SHRINK_HIGH_F_HIGH_H:
            cout << "high f/high h"; break;
        case SHRINK_RANDOM:
            cout << "random states"; break;
        case SHRINK_DFP:
            cout << "Draeger/Finkbeiner/Podelski"; break;
        default:
            abort();
    }
    cout << endl;
}

void MergeAndShrinkHeuristic::warn_on_unusual_options() const {
    if (use_expensive_statistics) {
        string dashes(79, '=');
        cerr << dashes << endl
             << ("WARNING! You have enabled extra statistics for "
                 "merge-and-shrink heuristics.\n"
                 "These statistics require a lot of time and memory.\n"
                 "When last tested (around revision 3011), enabling the "
                 "extra statistics\nincreased heuristic generation time by "
                 "76%. This figure may be significantly\nworse with more "
                 "recent code or for particular domains and instances.\n"
                 "You have been warned. Don't use this for benchmarking!")
             << endl << dashes << endl;
    }
}

void MergeAndShrinkHeuristic::verify_no_axioms_no_cond_effects() const {
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

Abstraction *MergeAndShrinkHeuristic::build_abstraction(bool is_first) {
    // TODO: We're leaking memory here in various ways. Fix this.
    //       Don't forget that build_atomic_abstractions also
    //       allocates memory.
    vector<Abstraction *> atomic_abstractions;
    cout << "Building atomic abstractions..." << endl;
    Abstraction::build_atomic_abstractions(atomic_abstractions);

    cout << "Merging abstractions..." << endl;
    int threshold = max_abstract_states;

    VariableOrderFinder order(merge_strategy, is_first);

    Abstraction *abstraction = atomic_abstractions[order.next()];
    abstraction->statistics(use_expensive_statistics);

    bool first_iteration = true;
    while(!order.done() && abstraction->is_solvable()) {
        int var_no = order.next();

        int max_allowed_size;
        int atomic_abstraction_target_size = atomic_abstractions[var_no]->size();
        if(bound_is_for_product) {
            int balanced_size = int(sqrt(threshold));
            if(atomic_abstraction_target_size > balanced_size)
                atomic_abstraction_target_size = balanced_size;
            max_allowed_size = threshold / atomic_abstraction_target_size;
        } else {
            max_allowed_size = threshold;
            if(atomic_abstraction_target_size > threshold)
                atomic_abstraction_target_size = threshold;
        }

        if(atomic_abstraction_target_size != atomic_abstractions[var_no]->size()) {
            cout << "atomic abstraction too big; must shrink" << endl;
            atomic_abstractions[var_no]->shrink(
                atomic_abstraction_target_size, shrink_strategy);
        }

        if(abstraction->size() > max_allowed_size) {
            abstraction->shrink(max_allowed_size, shrink_strategy);
            abstraction->statistics(use_expensive_statistics);
        }
        Abstraction *new_abstraction = new CompositeAbstraction(
            abstraction, atomic_abstractions[var_no],
            use_label_simplification);
        if(first_iteration)
            first_iteration = false;
        else
            abstraction->release_memory();
        abstraction = new_abstraction;
        abstraction->statistics(use_expensive_statistics);
    }
    return abstraction;
}

void MergeAndShrinkHeuristic::initialize() {
    int threshold = max_abstract_states;
    Timer timer;
    cout << "Initializing merge-and-shrink heuristic..." << endl;
    verify_no_axioms_no_cond_effects();
    cout << "Abstraction size limit: " << threshold << endl;

    int peak_memory = 0;
    for(int i = 0; i < abstraction_count; i++) {
        cout << "Building abstraction nr " << i << "..." << endl;
        Abstraction *abstraction = build_abstraction(i == 0);
        peak_memory = max(peak_memory, abstraction->get_peak_memory_estimate());
        abstractions.push_back(abstraction);
        if(!abstractions.back()->is_solvable())
            break;
    }

    cout << "Done initializing merge-and-shrink heuristic ["
         << timer << "]" << endl
         << "initial h value: " << compute_heuristic(*g_initial_state)
         << endl;
    cout << "Estimated peak memory: " << peak_memory << " bytes" << endl;
}

int MergeAndShrinkHeuristic::compute_heuristic(const State &state) {
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

ScalarEvaluator *MergeAndShrinkHeuristic::create(
    const std::vector<string> &config, int start, int &end) {
    int max_abstract_states = 1000;
    bool bound_is_for_product = true;
    int abstraction_count = 1;
    int merge_strategy = MERGE_LINEAR_CG_GOAL_LEVEL;
    int shrink_strategy = SHRINK_HIGH_F_LOW_H;
    bool use_label_simplification = true;
    bool use_expensive_statistics = false;

    // "<name>()" or "<name>(<options>)"
    if (config.size() > start + 2 && config[start + 1] == "(") {
        end = start + 2;

        // TODO: better documentation what each parameter does
        if (config[end] != ")") { 
            NamedOptionParser option_parser;
            option_parser.add_int_option(
                "max_states",
                &max_abstract_states,
                "maximum abstraction size");
            option_parser.add_int_option(
                "count",
                &abstraction_count, 
                "nr of abstractions to build");
            option_parser.add_int_option(
                "merge_strategy",
                &merge_strategy, 
                "merge strategy");
            option_parser.add_int_option(
                "shrink_strategy",
                &shrink_strategy,
                "shrink strategy");
            option_parser.add_bool_option(
                "bound_is_for_product",
                &bound_is_for_product,
                "merge and shrink bound is for product");
            option_parser.add_bool_option(
                "simplify_labels",
                &use_label_simplification,
                "enable label simplification");
            option_parser.add_bool_option(
                "expensive_statistics",
                &use_expensive_statistics,
                "show statistics on \"unique unlabeled edges\" (WARNING: "
                "these are *very* slow -- check the warning in the output)");
            option_parser.parse_options(config, end, end);
            end ++;
        }
        if (config[end] != ")") throw ParseError(end);
        
    } else { // "<name>"
        end = start;
    }

    if (max_abstract_states < 1) {
        cerr << "error: abstraction size must be at least 1"
             << endl;
        exit(2);
    }

    if (merge_strategy < 0 || merge_strategy >= MAX_MERGE_STRATEGY) {
        cerr << "Unknown merge strategy: " << merge_strategy << endl;
        exit(2);
    }

    if (shrink_strategy < 0 || shrink_strategy >= MAX_SHRINK_STRATEGY) {
        cerr << "Unknown shrink strategy: " << shrink_strategy << endl;
        exit(2);
    }

    MergeAndShrinkHeuristic *result = new MergeAndShrinkHeuristic(
        max_abstract_states,
        bound_is_for_product,
        abstraction_count,
        static_cast<MergeStrategy>(merge_strategy),
        static_cast<ShrinkStrategy>(shrink_strategy),
        use_label_simplification,
        use_expensive_statistics);
    return result;
}
