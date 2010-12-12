#ifndef ITERATED_SEARCH_H
#define ITERATED_SEARCH_H

#include "search_engine.h"
#include "globals.h"
#include "search_progress.h"
#include "option_parser.h"

class Options;

class IteratedSearch : public SearchEngine {
private:
    int phase;
    bool last_phase_found_solution;
    int best_bound;
    bool found_solution;

    SearchEngine *current_search;
    string current_search_name;

    const vector<ParseTree> engine_configs;
    bool pass_bound;
    bool repeat_last_phase;
    bool continue_on_fail;
    bool continue_on_solve;

    vector<SearchProgress> phase_statistics;
    vector<bool> phase_found_solution;
    vector<int> phase_solution_cost;

    SearchEngine *get_search_engine(int engine_config_start_index);
    SearchEngine *create_phase(int p);
    int step_return_value();

    virtual void initialize();
    virtual int step();
public:
    IteratedSearch(const Options &opts);
    virtual ~IteratedSearch();
    void statistics() const;
};

#endif
