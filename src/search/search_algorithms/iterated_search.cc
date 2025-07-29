#include "iterated_search.h"

#include "../plugins/plugin.h"
#include "../utils/component_errors.h"
#include "../utils/logging.h"

#include <iostream>

using namespace std;

namespace iterated_search {
IteratedSearch::IteratedSearch(
    vector<shared_ptr<TaskIndependentComponent<SearchAlgorithm>>> search_algorithms,
    bool pass_bound,
    bool repeat_last_phase,
    bool continue_on_fail,
    bool continue_on_solve,
    OperatorCost cost_type,
    int bound,
    double max_time,
    const string &name,
    utils::Verbosity verbosity,
    const shared_ptr<AbstractTask> &task
    ) : SearchAlgorithm(
            cost_type,
            bound,
            max_time,
            name,
            verbosity,
            task),
        search_algorithms(search_algorithms),
        pass_bound(pass_bound),
        repeat_last_phase(repeat_last_phase),
        continue_on_fail(continue_on_fail),
        continue_on_solve(continue_on_solve),
        phase(0),
        last_phase_found_solution(false),
        best_bound(bound),
        iterated_found_solution(false) {
 }



shared_ptr<SearchAlgorithm> IteratedSearch::create_current_phase() {
    int num_phases = search_algorithms.size();
    if (phase >= num_phases) {
        /* We've gone through all searches. We continue if
           repeat_last_phase is true, but *not* if we didn't find a
           solution the last time around, since then this search would
           just behave the same way again (assuming determinism, which
           we might not actually have right now, but strive for). So
           this overrides continue_on_fail.
        */
        if (repeat_last_phase && last_phase_found_solution) {
            log << "Starting search: '" << search_algorithms[search_algorithms.size() - 1]->get_description() << "'" << endl;
            return search_algorithms[search_algorithms.size() - 1]->get_task_specific(task, 1);
        } else {
            return nullptr;
        }
    }

    log << "Starting search: '" << search_algorithms[phase]->get_description() << "'" << endl;
    return search_algorithms[phase]->get_task_specific(task, 1);
}

SearchStatus IteratedSearch::step() {
    shared_ptr<SearchAlgorithm> current_search = create_current_phase();
    if (!current_search) {
        return found_solution() ? SOLVED : FAILED;
    }
    if (pass_bound && best_bound < current_search->get_bound()) {
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


TaskIndependentIteratedSearch::TaskIndependentIteratedSearch(
    vector<shared_ptr<TaskIndependentComponent<SearchAlgorithm>>> search_algorithms,
    bool pass_bound,
    bool repeat_last_phase,
    bool continue_on_fail,
    bool continue_on_solve,
    OperatorCost cost_type,
    int bound,
    double max_time,
    const string &name,
    utils::Verbosity verbosity
    )
    : TaskIndependentComponent<SearchAlgorithm>(
                                     name,
                                     verbosity
                                     ),
                                     bound(bound),
	cost_type(cost_type),
                                     max_time(max_time),
      search_algorithms(search_algorithms),
      pass_bound(pass_bound),
      repeat_last_phase(repeat_last_phase),
      continue_on_fail(continue_on_fail),
      continue_on_solve(continue_on_solve) {
}

std::shared_ptr<SearchAlgorithm> TaskIndependentIteratedSearch::create_task_specific(const shared_ptr <AbstractTask> &task,
                                                                                     [[maybe_unused]] unique_ptr <ComponentMap> &component_map,
                                                                                     [[maybe_unused]] int depth) const {
    return make_shared<IteratedSearch>(search_algorithms,
                                       pass_bound,
                                       repeat_last_phase,
                                       continue_on_fail,
                                       continue_on_solve,
                                       cost_type,
                                       bound,
                                       max_time,
                                       description,
                                       verbosity,
                                       task);
}


class IteratedSearchFeature
    : public plugins::TypedFeature<TaskIndependentComponent<SearchAlgorithm>, TaskIndependentIteratedSearch> {
public:
    IteratedSearchFeature() : TypedFeature("iterated") {
        document_title("Iterated search");
        document_synopsis("");

        add_list_option<shared_ptr<TaskIndependentComponent<SearchAlgorithm>>>(
            "search_algorithms",
            "list of search algorithms for each phase",
            "");
        add_option<bool>(
            "pass_bound",
            "use the bound of iterated search as a bound for its component "
            "search algorithms, unless these already have a lower bound set. "
            "The iterated search bound is tightened whenever a component finds "
            "a cheaper plan.",
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
        add_search_algorithm_options_to_feature(*this, "iterated");

        document_note(
            "Note 1",
            "We don't cache heuristic values between search iterations at"
            " the moment. If you perform a LAMA-style iterative search,"
            " heuristic values and other per-state information will be computed"
            " multiple times.");
        document_note(
            "Note 2",
            "The configuration\n```\n"
            "--search \"iterated([lazy_wastar([ipdb()],w=10), "
            "lazy_wastar([ipdb()],w=5), lazy_wastar([ipdb()],w=3), "
            "lazy_wastar([ipdb()],w=2), lazy_wastar([ipdb()],w=1)])\"\n"
            "```\nwould perform the preprocessing phase of the ipdb heuristic "
            "5 times (once before each iteration).\n\n"
            "To avoid this, use heuristic predefinition, which avoids "
            "duplicate preprocessing, as follows:\n```\n"
            "\"let(h,ipdb(),iterated([lazy_wastar([h],w=10), "
            "lazy_wastar([h],w=5), lazy_wastar([h],w=3), lazy_wastar([h],w=2), "
            "lazy_wastar([h],w=1)]))\"\n"
            "```");
    }



    virtual shared_ptr<TaskIndependentIteratedSearch> create_component(const plugins::Options &opts) const override {
        utils::verify_list_not_empty(opts.get_list<shared_ptr<TaskIndependentComponent<SearchAlgorithm>>>(
                "search_algorithms"), "search_algorithms");
        return make_shared<TaskIndependentIteratedSearch>(
            opts.get_list<shared_ptr<TaskIndependentComponent<SearchAlgorithm>>>(
                "search_algorithms"),
            opts.get<bool>("pass_bound"),
            opts.get<bool>("repeat_last"),
            opts.get<bool>("continue_on_fail"),
            opts.get<bool>("continue_on_solve"),
            opts.get<OperatorCost>("cost_type"),
            opts.get<int>("bound"),
            opts.get<double>("max_time"),
            opts.get<string>("description"),
            opts.get<utils::Verbosity>("verbosity")
            );
	}

};

static plugins::FeaturePlugin<IteratedSearchFeature> _plugin;
}
