#ifndef SEARCH_PROGRESS_H
#define SEARCH_PROGRESS_H

#include "globals.h"
#include "timer.h"
#include "heuristic.h"

/**
 * This class is a class that helps track search progress.
 * It keeps counters for expanded, generated, evaluated, ...
 * And it provides output in a uniform fashion for all search methods.
 */
class SearchProgress {
private:
    // General Statistics
    int expanded_states;  // nr states for which successors were generated
    int evaluated_states; // nr states for which h fn was computed
    int evaluations;      // nr of heuristic evaluations performed
    int generated_states; // nr states created in total (plus those removed since already in close list)
    int reopened_states;  // nr of *closed* states which we reopened
    int dead_end_states;

    int generated_ops;    // nr of operators that were returned as applicable
    int pathmax_corrections; // nr of pathmax corrections;

    // f-statistics
    int lastjump_f_value; //f value obtained in the last jump
    int lastjump_expanded_states; // same guy but at point where the last jump in the open list
    int lastjump_reopened_states; // occurred (jump == f-value of the first node in the queue increases)
    int lastjump_evaluated_states;
    int lastjump_generated_states;

    // h-statistics
    std::vector<int> best_heuristic_values; // best heuristic values so far
    std::vector<int> initial_h_values; // h values of the initial state
    std::vector<Heuristic *> heuristics;
public:
    SearchProgress();
    virtual ~SearchProgress();

    void add_heuristic(Heuristic *h);

    // statistics update
    void inc_expanded(int inc = 1) {expanded_states += inc; }
    void inc_evaluated_states(int inc = 1) {evaluated_states += inc; }
    void inc_generated(int inc = 1) {generated_states += inc; }
    void inc_reopened(int inc = 1) {reopened_states += inc; }
    void inc_generated_ops(int inc = 1) {generated_ops += inc; }
    void inc_pathmax_corrections(int inc = 1) {pathmax_corrections += inc; }
    void inc_evaluations(int inc = 1) {evaluations += inc; }
    void inc_dead_ends(int inc = 1) {dead_end_states += inc; }

    //statistics access
    int get_expanded() const {return expanded_states; }
    int get_evaluated_states() const {return evaluated_states; }
    int get_evaluations() const {return evaluations; }
    int get_generated() const {return generated_states; }
    int get_reopened() const {return reopened_states; }
    int get_generated_ops() const {return generated_ops; }
    int get_pathmax_corrections() const {return pathmax_corrections; }

    // f-value
    void report_f_value(int f);

    // h-value
    void get_initial_h_values();
    bool check_h_progress(int g);


    // output
    void print_line() const;
    void print_f_line() const;
    void print_h_line(int g) const;
    void print_statistics() const;
};

#endif
