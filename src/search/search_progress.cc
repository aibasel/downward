#include "search_progress.h"

#include "evaluation_context.h"
#include "globals.h"
#include "timer.h"
#include "utilities.h"

#include <iostream>
using namespace std;

SearchProgress::SearchProgress() {
    expanded_states = 0;
    reopened_states = 0;
    evaluated_states = 0;
    evaluations = 0;
    generated_states = 0;
    dead_end_states = 0;
    generated_ops = 0;
    pathmax_corrections = 0;

    lastjump_expanded_states = 0;
    lastjump_reopened_states = 0;
    lastjump_evaluated_states = 0;
    lastjump_generated_states = 0;

    lastjump_f_value = -1;
}

SearchProgress::~SearchProgress() {
}

void SearchProgress::add_heuristic(Heuristic *heur) {
    heuristics.push_back(heur);
}

void SearchProgress::report_f_value(int f) {
    if (f > lastjump_f_value) {
        lastjump_f_value = f;
        print_f_line();
        lastjump_expanded_states = expanded_states;
        lastjump_reopened_states = reopened_states;
        lastjump_evaluated_states = evaluated_states;
        lastjump_generated_states = generated_states;
    }
}

void SearchProgress::set_initial_h_values(EvaluationContext &eval_context) {
    assert(initial_heuristic_values.empty());
    assert(best_heuristic_values.empty());
    initial_heuristic_values.reserve(heuristics.size());
    best_heuristic_values.reserve(heuristics.size());
    for (Heuristic *heuristic : heuristics) {
        int h_value = eval_context.get_heuristic_value_or_infinity(heuristic);
        initial_heuristic_values.push_back(h_value);
        best_heuristic_values.push_back(h_value);
    }
    print_best_heuristic_values(0);
}

bool SearchProgress::check_h_progress(EvaluationContext &eval_context, int g) {
    assert(heuristics.size() == best_heuristic_values.size());
    bool progress = false;
    for (size_t i = 0; i < heuristics.size(); ++i) {
        int h = eval_context.get_heuristic_value_or_infinity(heuristics[i]);
        int &best_h = best_heuristic_values[i];
        if (h < best_h) {
            best_h = h;
            progress = true;
        }
    }
    if (progress)
        print_best_heuristic_values(g);
    return progress;
}

void SearchProgress::print_f_line() const {
    cout << "f = " << lastjump_f_value
         << " [";
    print_line();
    cout << "]" << endl;
}

void SearchProgress::print_best_heuristic_values(int g) const {
    cout << "Best heuristic value: ";
    for (size_t i = 0; i < best_heuristic_values.size(); ++i) {
        if (i != 0)
            cout << "/";
        cout << best_heuristic_values[i];
    }
    cout << " [g=" << g << ", ";
    print_line();
    cout << "]" << endl;
}

void SearchProgress::print_line() const {
    cout << evaluated_states << " evaluated, "
         << expanded_states << " expanded, ";
    if (reopened_states > 0) {
        cout << reopened_states << " reopened, ";
    }
    cout << "t=" << g_timer;
    cout << ", " << get_peak_memory_in_kb() << " KB";
}

void SearchProgress::print_statistics() const {
    if (!initial_heuristic_values.empty()) {
        // This will be skipped in the cumulative statistics of an
        // iterated search, which do not have initial h values.
        cout << "Initial state h value: ";
        for (size_t i = 0; i < initial_heuristic_values.size(); ++i) {
            if (i != 0)
                cout << "/";
            cout << initial_heuristic_values[i];
        }
        cout << "." << endl;
    }

    cout << "Expanded " << expanded_states << " state(s)." << endl;
    cout << "Reopened " << reopened_states << " state(s)." << endl;
    cout << "Evaluated " << evaluated_states << " state(s)." << endl;
    cout << "Evaluations: " << evaluations << endl;
    cout << "Generated " << generated_states << " state(s)." << endl;
    cout << "Dead ends: " << dead_end_states << " state(s)." << endl;
    if (pathmax_corrections > 0) {
        cout << "Pathmax corrections: " << pathmax_corrections << endl;
    }

    if (lastjump_f_value >= 0) {
        cout << "Expanded until last jump: "
             << lastjump_expanded_states << " state(s)." << endl;
        cout << "Reopened until last jump: "
             << lastjump_reopened_states << " state(s)." << endl;
        cout << "Evaluated until last jump: "
             << lastjump_evaluated_states << " state(s)." << endl;
        cout << "Generated until last jump: "
             << lastjump_generated_states << " state(s)." << endl;
    }
}
