#ifndef ITERATED_SEARCH_H
#define ITERATED_SEARCH_H

#include "search_engine.h"
#include "globals.h"
#include "search_progress.h"
#include "option_parser.h"

class IteratedSearch : public SearchEngine {
    int phase;
    bool last_phase_found_solution;
    int best_bound;
    bool iterated_found_solution;
    int plan_counter;

    SearchEngine *current_search;
    string current_search_name;

    const vector<string> engine_config;
    vector<int> engine_config_start;
    bool pass_bound;
    bool repeat_last_phase;
    bool continue_on_fail;
    bool continue_on_solve;

    SearchEngine *get_search_engine(int engine_config_start_index);
    SearchEngine *create_phase(int p);
    int step_return_value();

    virtual void initialize();
    virtual int step();
public:
    IteratedSearch(const SearchEngineOptions &options,
                   const std::vector<std::string> &engine_config,
                   std::vector<int> engine_config_start,
                   bool pass_bound,
                   bool repeat_last_phase,
                   bool continue_on_fail,
                   bool continue_on_solve,
                   int plan_counter);
    virtual ~IteratedSearch();
    virtual void save_plan_if_necessary() const;
    void statistics() const;
    static SearchEngine *create(
        const std::vector<std::string> &config, int start, int &end,
        bool dry_run);
};

#endif
