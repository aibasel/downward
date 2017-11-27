#ifndef SEARCH_ENGINES_ITERATED_SEARCH_H
#define SEARCH_ENGINES_ITERATED_SEARCH_H

#include "../option_parser_util.h"
#include "../search_engine.h"

namespace options {
class Options;
}

namespace iterated_search {
class IteratedSearch : public SearchEngine {
    const std::vector<options::ParseTree> engine_configs;
    bool pass_bound;
    bool repeat_last_phase;
    bool continue_on_fail;
    bool continue_on_solve;

    int phase;
    bool last_phase_found_solution;
    int best_bound;
    bool iterated_found_solution;

    std::shared_ptr<SearchEngine> get_search_engine(int engine_config_start_index);
    std::shared_ptr<SearchEngine> create_phase(int phase);
    SearchStatus step_return_value();

    virtual SearchStatus step() override;

public:
    explicit IteratedSearch(const options::Options &opts);

    virtual void save_plan_if_necessary() const override;
    virtual void print_statistics() const override;
};
}

#endif
