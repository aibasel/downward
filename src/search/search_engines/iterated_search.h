#ifndef SEARCH_ENGINES_ITERATED_SEARCH_H
#define SEARCH_ENGINES_ITERATED_SEARCH_H

#include "../search_engine.h"

#include "../plugins/plugin.h"
#include "../plugins/predefinitions.h"
#include "../plugins/registries.h"

namespace plugins {
class Options;
}

namespace iterated_search {
class IteratedSearch : public SearchEngine {
    const std::vector<plugins::ParseTree> engine_configs;
    /*
      We need to copy the registry and predefinitions here since they live
      longer than the objects referenced in the constructor.
    */
    plugins::Registry registry;
    plugins::Predefinitions predefinitions;
    bool pass_bound;
    bool repeat_last_phase;
    bool continue_on_fail;
    bool continue_on_solve;

    int phase;
    bool last_phase_found_solution;
    int best_bound;
    bool iterated_found_solution;

    std::shared_ptr<SearchEngine> get_search_engine(int engine_configs_index);
    std::shared_ptr<SearchEngine> create_current_phase();
    SearchStatus step_return_value();

    virtual SearchStatus step() override;

public:
    IteratedSearch(const plugins::Options &opts, plugins::Registry &registry,
                   const plugins::Predefinitions &predefinitions);

    virtual void save_plan_if_necessary() override;
    virtual void print_statistics() const override;
};
}

#endif
