#ifndef ITERATED_SEARCH_H
#define ITERATED_SEARCH_H

#include "search_engine.h"
#include "globals.h"
#include "search_progress.h"
#include "option_parser.h"

class Options;

class IteratedSearch : public SearchEngine {
    int phase;
    bool last_phase_found_solution;
    int best_bound;
    bool iterated_found_solution;

    SearchEngine *current_search;
    std::string current_search_name;

    const std::vector<ParseTree> engine_configs;
    bool pass_bound;
    bool repeat_last_phase;
    bool continue_on_fail;
    bool continue_on_solve;

    SearchEngine *get_search_engine(int engine_config_start_index);
    SearchEngine *create_phase(int p);
    SearchStatus step_return_value();

    virtual void initialize();
    virtual SearchStatus step();
public:
    IteratedSearch(const Options &opts);
    virtual ~IteratedSearch();
    virtual void save_plan_if_necessary() const;
    void statistics() const;
};

#endif
