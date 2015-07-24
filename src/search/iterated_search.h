#ifndef ITERATED_SEARCH_H
#define ITERATED_SEARCH_H

#include "option_parser_util.h"
#include "search_engine.h"

class Options;

class IteratedSearch : public SearchEngine {
    const std::vector<ParseTree> engine_configs;
    bool pass_bound;
    bool repeat_last_phase;
    bool continue_on_fail;
    bool continue_on_solve;

    int phase;
    bool last_phase_found_solution;
    int best_bound;
    bool iterated_found_solution;

    SearchEngine *current_search;
    std::string current_search_name;

    SearchEngine *get_search_engine(int engine_config_start_index);
    SearchEngine *create_phase(int p);
    SearchStatus step_return_value();

    virtual void initialize() override;
    virtual SearchStatus step() override;

public:
    explicit IteratedSearch(const Options &opts);
    virtual ~IteratedSearch() override = default;

    virtual void save_plan_if_necessary() const override;
    virtual void print_statistics() const override;
};

#endif
