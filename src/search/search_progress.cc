#include "search_progress.h"

#include "evaluation_context.h"
#include "search_statistics.h"

#include <iostream>
using namespace std;


void SearchProgress::add_heuristic(Heuristic *heur) {
    heuristics.push_back(heur);
}

void SearchProgress::set_initial_h_values(
    EvaluationContext &eval_context, const SearchStatistics &statistics) {
    assert(initial_heuristic_values.empty());
    assert(best_heuristic_values.empty());
    initial_heuristic_values.reserve(heuristics.size());
    best_heuristic_values.reserve(heuristics.size());
    for (Heuristic *heuristic : heuristics) {
        int h_value = eval_context.get_heuristic_value_or_infinity(heuristic);
        initial_heuristic_values.push_back(h_value);
        best_heuristic_values.push_back(h_value);
    }
    print_best_heuristic_values(0, statistics);
}

bool SearchProgress::check_h_progress(
    EvaluationContext &eval_context, int g,
    const SearchStatistics &statistics) {
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
        print_best_heuristic_values(g, statistics);
    return progress;
}

void SearchProgress::print_best_heuristic_values(
    int g, const SearchStatistics &statistics) const {
    cout << "Best heuristic value: ";
    for (size_t i = 0; i < best_heuristic_values.size(); ++i) {
        if (i != 0)
            cout << "/";
        cout << best_heuristic_values[i];
    }
    cout << " [g=" << g << ", ";
    statistics.print_basic_statistics();
    cout << "]" << endl;
}

void SearchProgress::print_initial_h_values() const {
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
}
