#include "iterated_search.h"

#include "../plugins/plugin.h"
#include "../utils/logging.h"

#include <iostream>

using namespace std;

namespace iterated_search {
IteratedSearch::IteratedSearch(const plugins::Options &opts)
    : SearchAlgorithm(opts),
      algorithm_configs(opts.get_list<parser::LazyValue>("algorithm_configs")),
      pass_bound(opts.get<bool>("pass_bound")),
      repeat_last_phase(opts.get<bool>("repeat_last")),
      continue_on_fail(opts.get<bool>("continue_on_fail")),
      continue_on_solve(opts.get<bool>("continue_on_solve")),
      phase(0),
      last_phase_found_solution(false),
      best_bound(bound),
      iterated_found_solution(false) {
}

shared_ptr<SearchAlgorithm> IteratedSearch::get_search_algorithm(
    int algorithm_configs_index) {
    parser::LazyValue &algorithm_config = algorithm_configs[algorithm_configs_index];
    shared_ptr<SearchAlgorithm> search_algorithm;
    try{
        search_algorithm = algorithm_config.construct<shared_ptr<SearchAlgorithm>>();
    } catch (const utils::ContextError &e) {
        cerr << "Delayed construction of LazyValue failed" << endl;
        cerr << e.get_message() << endl;
        utils::exit_with(utils::ExitCode::SEARCH_INPUT_ERROR);
    }
    log << "Starting search: " << search_algorithm->get_description() << endl;
    return search_algorithm;
}

shared_ptr<SearchAlgorithm> IteratedSearch::create_current_phase() {
    int num_phases = algorithm_configs.size();
    if (phase >= num_phases) {
        /* We've gone through all searches. We continue if
           repeat_last_phase is true, but *not* if we didn't find a
           solution the last time around, since then this search would
           just behave the same way again (assuming determinism, which
           we might not actually have right now, but strive for). So
           this overrides continue_on_fail.
        */
        if (repeat_last_phase && last_phase_found_solution) {
            return get_search_algorithm(
                algorithm_configs.size() -
                1);
        } else {
            return nullptr;
        }
    }

    return get_search_algorithm(phase);
}

SearchStatus IteratedSearch::step() {
    shared_ptr<SearchAlgorithm> current_search = create_current_phase();
    if (!current_search) {
        return found_solution() ? SOLVED : FAILED;
    }
    if (pass_bound) {
        current_search->set_bound(best_bound);
    }
    ++phase;

    current_search->search();

    Plan found_plan;
    int plan_cost = 0;
    last_phase_found_solution = current_search->found_solution();
    if (last_phase_found_solution) {
        iterated_found_solution = true;
        found_plan = current_search->get_plan();
        plan_cost = calculate_plan_cost(found_plan, task_proxy);
        if (plan_cost < best_bound) {
            plan_manager.save_plan(found_plan, task_proxy, true);
            best_bound = plan_cost;
            set_plan(found_plan);
        }
    }
    current_search->print_statistics();

    const SearchStatistics &current_stats = current_search->get_statistics();
    statistics.inc_expanded(current_stats.get_expanded());
    statistics.inc_evaluated_states(current_stats.get_evaluated_states());
    statistics.inc_evaluations(current_stats.get_evaluations());
    statistics.inc_generated(current_stats.get_generated());
    statistics.inc_generated_ops(current_stats.get_generated_ops());
    statistics.inc_reopened(current_stats.get_reopened());

    return step_return_value();
}

SearchStatus IteratedSearch::step_return_value() {
    if (iterated_found_solution)
        log << "Best solution cost so far: " << best_bound << endl;

    if (last_phase_found_solution) {
        if (continue_on_solve) {
            log << "Solution found - keep searching" << endl;
            return IN_PROGRESS;
        } else {
            log << "Solution found - stop searching" << endl;
            return SOLVED;
        }
    } else {
        if (continue_on_fail) {
            log << "No solution found - keep searching" << endl;
            return IN_PROGRESS;
        } else {
            log << "No solution found - stop searching" << endl;
            return iterated_found_solution ? SOLVED : FAILED;
        }
    }
}

void IteratedSearch::print_statistics() const {
    log << "Cumulative statistics:" << endl;
    statistics.print_detailed_statistics();
}

void IteratedSearch::save_plan_if_necessary() {
    // We don't need to save here, as we automatically save after
    // each successful search iteration.
}

class IteratedSearchFeature : public plugins::TypedFeature<SearchAlgorithm, IteratedSearch> {
public:
    IteratedSearchFeature() : TypedFeature("iterated") {
        document_title("Iterated search");
        document_synopsis("");

        add_list_option<shared_ptr<SearchAlgorithm>>(
            "algorithm_configs",
            "list of search algorithms for each phase",
            "",
            true);
        add_option<bool>(
            "pass_bound",
            "use bound from previous search. The bound is the real cost "
            "of the plan found before, regardless of the cost_type parameter.",
            "true");
        add_option<bool>(
            "repeat_last",
            "repeat last phase of search",
            "false");
        add_option<bool>(
            "continue_on_fail",
            "continue search after no solution found",
            "false");
        add_option<bool>(
            "continue_on_solve",
            "continue search after solution found",
            "true");
        SearchAlgorithm::add_options_to_feature(*this);

        document_note(
            "Note 1",
            "We don't cache heuristic values between search iterations at"
            " the moment. If you perform a LAMA-style iterative search,"
            " heuristic values will be computed multiple times.");
        document_note(
            "Note 2",
            "The configuration\n```\n"
            "--search \"iterated([lazy_wastar([ipdb()],w=10), "
            "lazy_wastar([ipdb()],w=5), lazy_wastar([ipdb()],w=3), "
            "lazy_wastar([ipdb()],w=2), lazy_wastar([ipdb()],w=1)])\"\n"
            "```\nwould perform the preprocessing phase of the ipdb heuristic "
            "5 times (once before each iteration).\n\n"
            "To avoid this, use heuristic predefinition, which avoids duplicate "
            "preprocessing, as follows:\n```\n"
            "--evaluator \"h=ipdb()\" --search "
            "\"iterated([lazy_wastar([h],w=10), lazy_wastar([h],w=5), lazy_wastar([h],w=3), "
            "lazy_wastar([h],w=2), lazy_wastar([h],w=1)])\"\n"
            "```");
        document_note(
            "Note 3",
            "If you reuse the same landmark count heuristic "
            "(using heuristic predefinition) between iterations, "
            "the path data (that is, landmark status for each visited state) "
            "will be saved between iterations.");
    }

    virtual shared_ptr<IteratedSearch> create_component(const plugins::Options &options, const utils::Context &context) const override {
        plugins::Options options_copy(options);
        /*
          The options entry 'algorithm_configs' is a LazyValue representing a list
          of search algorithms. But iterated search expects a list of LazyValues,
          each representing a search algorithm. We unpack this first layer of
          laziness here to report potential errors in a more useful context.

          TODO: the medium-term plan is to get rid of LazyValue completely
          and let the features create builders that in turn create the actual
          search algorithms. Then we no longer need to be lazy because creating
          the builder is a light-weight operation.
        */
        vector<parser::LazyValue> algorithm_configs =
            options.get<parser::LazyValue>("algorithm_configs").construct_lazy_list();
        options_copy.set("algorithm_configs", algorithm_configs);
        plugins::verify_list_non_empty<parser::LazyValue>(context, options_copy, "algorithm_configs");
        return make_shared<IteratedSearch>(options_copy);
    }
};

static plugins::FeaturePlugin<IteratedSearchFeature> _plugin;
}
