#include "maximum_heuristic.h"

#include "../global_state.h"
#include "../globals.h"
#include "../option_parser.h"
#include "../search_space.h"

#include <cassert>
#include <limits>

MaxHeuristic::MaxHeuristic(const Options &opts, bool arff)
    : Heuristic(opts), num_evals(0), arff_out("max.arff") {
    //name = "max";
    dump_arff = arff;
    max_diff = 5;
}

MaxHeuristic::~MaxHeuristic() {
}

void MaxHeuristic::initialize() {
    if (dump_arff) {
        arff_out << "@RELATION max" << endl;
        arff_out << endl;

        for (size_t i = 0; i < g_variable_domain.size(); ++i) {
            arff_out << "@ATTRIBUTE " << g_variable_name[i] << " {";
            arff_out << "0";
            for (int j = 1; j < g_variable_domain[i]; ++j) {
                arff_out << "," << j;
            }
            arff_out << "}" << endl;
        }
        //arff_out << "@ATTRIBUTE g NUMERIC" << endl;

        //arff_out << "@ATTRIBUTE f NUMERIC" << endl;

        /*
        for (size_t i = 0; i < heuristics.size(); ++i) {
                arff_out << "@ATTRIBUTE h" << i << " NUMERIC" << endl;
        }
        */

        /*
        arff_out << "@ATTRIBUTE winner {0";
        for (size_t i = 0; i < heuristics.size(); ++i) {
                arff_out << "," <<  (i+1) ;
        }
        arff_out << "}" << endl;
        */

        arff_out << endl;
        arff_out << "@DATA" << endl;
    }
}


int MaxHeuristic::compute_heuristic(const GlobalState &state) {
    int max = 0;
    bool dead_end = false;
    ++num_evals;

    for (size_t i = 0; i < heuristics.size(); ++i) {
        //cout << "h[" << i << "] = ";
        heuristics[i]->evaluate(state);
        if (heuristics[i]->is_dead_end()) {
            if (heuristics[i]->dead_ends_are_reliable()) {
                return DEAD_END;
            } else {
                hvalue[i] = numeric_limits<int>::max();
                dead_end = true;
            }
        } else {
            hvalue[i] = heuristics[i]->get_heuristic();
            if (hvalue[i] > max) {
                max = hvalue[i];
            }
        }
    }


    if (dead_end) {
        bool all_dead_end = true;
        for (size_t i = 0; i < heuristics.size(); ++i) {
            if (!heuristics[i]->is_dead_end()) {
                all_dead_end = false;
                break;
            }
        }
        if (all_dead_end) {
            return DEAD_END;
        }
    }

    int winner_count = 0;
    int winner_id = -1;
    for (size_t i = 0; i < heuristics.size(); ++i) {
        if (hvalue[i] == max) {
            ++winner[i];
            ++winner_count;
            winner_id = i;
        }
    }

    if (dump_arff) {
        for (size_t i = 0; i < g_variable_domain.size(); ++i) {
            arff_out << state[i] << ", ";
        }
        //int g = g_learning_search_space->get_node(state).get_g();
        //arff_out << g << ", " << max + g << endl;
    }

    if (winner_count == 1) {
        ++only_winner[winner_id];
    }

    return max;
}

void MaxHeuristic::print_statistics() const {
    cout << "Num evals: " << num_evals << endl;
    cout << "heuristic,  winner,   only winner" << endl;
    for (size_t i = 0; i < heuristics.size(); ++i) {
        cout << i /*heuristics[i]->get_name()*/ << " , " << winner[i] << " , " << only_winner[i] << endl;
    }
}

bool MaxHeuristic::reach_state(const GlobalState &parent_state, const GlobalOperator &op,
                               const GlobalState &state) {
    int ret = false;
    int val;
    for (size_t i = 0; i < heuristics.size(); ++i) {
        val = heuristics[i]->reach_state(parent_state, op, state);
        ret = ret || val;
    }
    return ret;
}
