#ifndef SEARCH_ALGORITHMS_ITERATED_SEARCH_H
#define SEARCH_ALGORITHMS_ITERATED_SEARCH_H

#include "../search_algorithm.h"

#include "../parser/decorated_abstract_syntax_tree.h"

#include <memory>
#include <vector>

namespace iterated_search {
class HackPlaceholder {
public:
    HackPlaceholder() {
        std::cout << "HACK :-(" << std::endl;
    }
};
using IteratedSearchArgs = WrapArgs<
    std::pair<
        std::vector<
            std::shared_ptr<TaskIndependentComponentType<SearchAlgorithm>>>,
        HackPlaceholder>, // search_algorithms,
    // HACK the pair is just used to prevent the searches from being specified
    // when the outer iterated search is being specified.
    bool, // pass_bound,
    bool, // repeat_last_phase,
    bool, // continue_on_fail,
    bool, // continue_on_solve,
    OperatorCost, // cost_type,
    int, // bound,
    double, // max_time,
    const std::string, //&name,
    utils::Verbosity // verbosity
    >;

class IteratedSearch : public SearchAlgorithm {
    std::vector<std::shared_ptr<TaskIndependentComponentType<SearchAlgorithm>>>
        search_algorithms;

    bool pass_bound;
    bool repeat_last_phase;
    bool continue_on_fail;
    bool continue_on_solve;

    int phase;
    bool last_phase_found_solution;
    int best_bound;
    bool iterated_found_solution;

    std::shared_ptr<SearchAlgorithm> get_search_algorithm(
        int algorithm_configs_index);
    std::shared_ptr<SearchAlgorithm> create_current_phase();
    SearchStatus step_return_value();

    virtual SearchStatus step() override;

public:
    IteratedSearch(
        const std::shared_ptr<AbstractTask> &task,
        std::pair<
            std::vector<
                std::shared_ptr<TaskIndependentComponentType<SearchAlgorithm>>>,
            HackPlaceholder>
            search_algorithms,
        bool pass_bound, bool repeat_last_phase, bool continue_on_fail,
        bool continue_on_solve, OperatorCost cost_type, int bound,
        double max_time, const std::string &name, utils::Verbosity verbosity);

    virtual void save_plan_if_necessary() override;
    virtual void print_statistics() const override;
};
}

#endif
