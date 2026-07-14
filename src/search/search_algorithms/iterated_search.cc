#include "iterated_search.h"

#include "../evaluator.h"

#include "../landmarks/landmark_factory.h"
#include "../plugins/plugin.h"
#include "../utils/component_errors.h"
#include "../utils/logging.h"

#include <iostream>

using namespace std;

namespace iterated_search {
IteratedSearch::IteratedSearch(
    const shared_ptr<AbstractTask> &task,
    const vector<shared_ptr<TaskIndependentSearchAlgorithm>> &search_algorithms,
    bool pass_bound, bool repeat_last, bool continue_on_fail,
    bool continue_on_solve, OperatorCost cost_type, int bound, double max_time,
    const string &description, utils::Verbosity verbosity)
    : TaskSpecificSearchAlgorithm(
          task, cost_type, bound, max_time, description, verbosity),
      task_independent_searches(search_algorithms),
      pass_bound(pass_bound),
      repeat_last_phase(repeat_last),
      continue_on_fail(continue_on_fail),
      continue_on_solve(continue_on_solve),
      phase(0),
      last_phase_found_solution(false),
      best_bound(bound),
      iterated_found_solution(false) {
    utils::verify_list_not_empty(search_algorithms, "search_algorithms");
}

void IteratedSearch::update_retention_set() {
    unordered_set<TIComponent *> seen;
    vector<shared_ptr<TSComponent>> new_retained_components;
    /*
      Loop through all future phases. If repeat_last_phase is true, then the
      last phase always is considered a future phase, even if we currently are
      in the last phase.
    */
    assert(!task_independent_searches.empty());
    int start = phase;
    int end = task_independent_searches.size();
    if (start >= end && repeat_last_phase) {
        start = end - 1;
    }
    for (int i = start; i < end; ++i) {
        vector<TIComponent *> future_ti_components;
        future_ti_components.push_back(task_independent_searches[i].get());
        task_independent_searches[i]->get_task_preserving_subcomponents(
            future_ti_components);
        for (TIComponent *ti_component : future_ti_components) {
            if (!seen.contains(ti_component)) {
                seen.insert(ti_component);
                shared_ptr<TSComponent> ts_component =
                    ti_component->get_cached(task);
                if (ts_component) {
                    /*
                      HACK: so far, we only re-use evaluators and landmark
                      factories, as our search algorithms do not support being
                      used more than once.
                    */
                    const TaskSpecificEvaluator *evaluator =
                        dynamic_cast<const TaskSpecificEvaluator *>(
                            ts_component.get());
                    const landmarks::TaskSpecificLandmarkFactory *landmarks =
                        dynamic_cast<
                            const landmarks::TaskSpecificLandmarkFactory *>(
                            ts_component.get());
                    if (evaluator || landmarks) {
                        new_retained_components.push_back(ts_component);
                    }
                }
            }
        }
    }
    /*
      It is important to collect the retained components in a copy of
      retained_components, rather than clearing the vector and using it
      directly. This could cause retained components to go out of scope and
      be destroyed before the new entries are computed.
    */
    retained_components.swap(new_retained_components);
}

shared_ptr<TaskSpecificSearchAlgorithm> IteratedSearch::bind_search(
    int search_index) {
    shared_ptr<TaskIndependentSearchAlgorithm> &task_indepenent_search =
        task_independent_searches[search_index];
    shared_ptr<TaskSpecificSearchAlgorithm> task_specific_search =
        task_indepenent_search->bind_task(task);
    log << "Starting search: "
        << task_specific_search->get_description() << endl;
    return task_specific_search;
}

shared_ptr<TaskSpecificSearchAlgorithm> IteratedSearch::bind_current_search() {
    int num_phases = task_independent_searches.size();
    if (phase >= num_phases) {
        /* We've gone through all searches. We continue if
           repeat_last_phase is true, but *not* if we didn't find a
           solution the last time around, since then this search would
           just behave the same way again (assuming determinism, which
           we might not actually have right now, but strive for). So
           this overrides continue_on_fail.
        */
        if (repeat_last_phase && last_phase_found_solution) {
            return bind_search(num_phases - 1);
        } else {
            return nullptr;
        }
    }

    return bind_search(phase);
}

SearchStatus IteratedSearch::step() {
    shared_ptr<TaskSpecificSearchAlgorithm> task_specific_search =
        bind_current_search();
    if (!task_specific_search) {
        return found_solution() ? SOLVED : FAILED;
    }
    if (pass_bound && best_bound < task_specific_search->get_bound()) {
        task_specific_search->set_bound(best_bound);
    }
    ++phase;

    task_specific_search->search();

    Plan found_plan;
    int plan_cost = 0;
    last_phase_found_solution = task_specific_search->found_solution();
    if (last_phase_found_solution) {
        iterated_found_solution = true;
        found_plan = task_specific_search->get_plan();
        plan_cost = calculate_plan_cost(found_plan, task_proxy);
        if (plan_cost < best_bound) {
            plan_manager.save_plan(found_plan, task_proxy, true);
            best_bound = plan_cost;
            set_plan(found_plan);
        }
    }
    task_specific_search->print_statistics();

    const SearchStatistics &current_stats = task_specific_search->get_statistics();
    statistics.inc_expanded(current_stats.get_expanded());
    statistics.inc_evaluated_states(current_stats.get_evaluated_states());
    statistics.inc_evaluations(current_stats.get_evaluations());
    statistics.inc_generated(current_stats.get_generated());
    statistics.inc_generated_ops(current_stats.get_generated_ops());
    statistics.inc_reopened(current_stats.get_reopened());

    /*
      It is important to first update the retention set, then drop shared
      pointers for the current task-specific search and the current
      task-independent search. (Note that the last phase might be repeated. In
      that case, we cannot delete the task-independent search.)
    */
    update_retention_set();
    task_specific_search = nullptr;
    int num_phases = task_independent_searches.size();
    if (phase < num_phases || !repeat_last_phase ||
        !last_phase_found_solution) {
        int index = min(phase - 1, num_phases - 1);
        task_independent_searches.at(index) = nullptr;
    }

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

class TaskIndependentIteratedSearch
    : public components::TaskIndependentComponent<TaskSpecificSearchAlgorithm> {
    vector<shared_ptr<TaskIndependentSearchAlgorithm>> search_algorithms;
    bool pass_bound;
    bool repeat_last_phase;
    bool continue_on_fail;
    bool continue_on_solve;
    OperatorCost cost_type;
    int bound;
    double max_time;
    string description;
    utils::Verbosity verbosity;
protected:
    virtual shared_ptr<TaskSpecificSearchAlgorithm>
    create_task_specific_component(const shared_ptr<AbstractTask> &task) const {
        return make_shared<IteratedSearch>(
            task, search_algorithms, pass_bound, repeat_last_phase,
            continue_on_fail, continue_on_solve, cost_type, bound, max_time,
            description, verbosity);
    }

public:
    TaskIndependentIteratedSearch(
        const vector<shared_ptr<TaskIndependentSearchAlgorithm>>
            &search_algorithms,
        bool pass_bound, bool repeat_last, bool continue_on_fail,
        bool continue_on_solve, OperatorCost cost_type, int bound,
        double max_time, const string &description, utils::Verbosity verbosity)
        : search_algorithms(search_algorithms),
          pass_bound(pass_bound),
          repeat_last_phase(repeat_last),
          continue_on_fail(continue_on_fail),
          continue_on_solve(continue_on_solve),
          cost_type(cost_type),
          bound(bound),
          max_time(max_time),
          description(description),
          verbosity(verbosity) {
    }
};

class IteratedSearchFeature
    : public plugins::TypedFeature<TaskIndependentSearchAlgorithm> {
public:
    IteratedSearchFeature() : TypedFeature("iterated") {
        document_title("Iterated search");
        document_synopsis("");

        add_list_option<shared_ptr<TaskIndependentSearchAlgorithm>>(
            "search_algorithms", "list of search algorithms for each phase",
            "");
        add_option<bool>(
            "pass_bound",
            "use the bound of iterated search as a bound for its component "
            "search algorithms, unless these already have a lower bound set. "
            "The iterated search bound is tightened whenever a component finds "
            "a cheaper plan.",
            "true");
        add_option<bool>("repeat_last", "repeat last phase of search", "false");
        add_option<bool>(
            "continue_on_fail", "continue search after no solution found",
            "false");
        add_option<bool>(
            "continue_on_solve", "continue search after solution found",
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
            "--search \"iterated([lazy_wastar([ipdb()],w=10), lazy_wastar([ipdb()],w=5),\n"
            "                    lazy_wastar([ipdb()],w=3), lazy_wastar([ipdb()],w=2),\n"
            "                    lazy_wastar([ipdb()],w=1)])\"\n"
            "```\nwould perform the preprocessing phase of the ipdb heuristic "
            "5 times (once before each iteration).\n\n"
            "To avoid this, use heuristic predefinition, which avoids "
            "duplicate preprocessing, as follows:\n```\n"
            "\"let(h,ipdb(),iterated([lazy_wastar([h],w=10), lazy_wastar([h],w=5),\n"
            "                        lazy_wastar([h],w=3), lazy_wastar([h],w=2),\n"
            "                        lazy_wastar([h],w=1)]))\"\n"
            "```");
    }

    virtual shared_ptr<TaskIndependentSearchAlgorithm> create_component(
        const plugins::Options &opts) const override {
        return components::make_shared_from_arg_tuples<
            TaskIndependentIteratedSearch>(
            opts.get_list<shared_ptr<TaskIndependentSearchAlgorithm>>(
                "search_algorithms"),
            opts.get<bool>("pass_bound"), opts.get<bool>("repeat_last"),
            opts.get<bool>("continue_on_fail"),
            opts.get<bool>("continue_on_solve"),
            get_search_algorithm_arguments_from_options(opts));
    }
};

static plugins::FeaturePlugin<IteratedSearchFeature> _plugin;
}
