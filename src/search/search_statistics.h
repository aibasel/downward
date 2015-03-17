#ifndef SEARCH_STATISTICS_H
#define SEARCH_STATISTICS_H

/*
  This class keeps track of search statistics.

  It keeps counters for expanded, generated and evaluated states (and
  some other statistics) and provides uniform output for all search
  methods.
*/

class SearchStatistics {
    // General statistics
    int expanded_states;  // no states for which successors were generated
    int evaluated_states; // no states for which h fn was computed
    int evaluations;      // no of heuristic evaluations performed
    int generated_states; // no states created in total (plus those removed since already in close list)
    int reopened_states;  // no of *closed* states which we reopened
    int dead_end_states;

    int generated_ops;    // no of operators that were returned as applicable
    int pathmax_corrections; // no of pathmax corrections;

    // Statistics related to f values
    int lastjump_f_value; //f value obtained in the last jump
    int lastjump_expanded_states; // same guy but at point where the last jump in the open list
    int lastjump_reopened_states; // occurred (jump == f-value of the first node in the queue increases)
    int lastjump_evaluated_states;
    int lastjump_generated_states;

public:
    SearchStatistics();
    ~SearchStatistics() = default;

    // Methods that update statistics.
    void inc_expanded(int inc = 1) {expanded_states += inc; }
    void inc_evaluated_states(int inc = 1) {evaluated_states += inc; }
    void inc_generated(int inc = 1) {generated_states += inc; }
    void inc_reopened(int inc = 1) {reopened_states += inc; }
    void inc_generated_ops(int inc = 1) {generated_ops += inc; }
    void inc_pathmax_corrections(int inc = 1) {pathmax_corrections += inc; }
    void inc_evaluations(int inc = 1) {evaluations += inc; }
    void inc_dead_ends(int inc = 1) {dead_end_states += inc; }

    // Methods that access statistics.
    int get_expanded() const {return expanded_states; }
    int get_evaluated_states() const {return evaluated_states; }
    int get_evaluations() const {return evaluations; }
    int get_generated() const {return generated_states; }
    int get_reopened() const {return reopened_states; }
    int get_generated_ops() const {return generated_ops; }
    int get_pathmax_corrections() const {return pathmax_corrections; }

    // record jumps and possibly output
    void report_f_value(int f);

    // output only
    void print_f_line() const;
    void print_basic_statistics() const;
    void print_detailed_statistics() const;
};

#endif
