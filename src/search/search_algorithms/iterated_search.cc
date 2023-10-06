#include "iterated_search.h"

#include "../plugins/plugin.h"
#include "../utils/logging.h"

#include <iostream>

using namespace std;

namespace iterated_search {
IteratedSearch::IteratedSearch(utils::Verbosity verbosity,
                               OperatorCost cost_type,
                               double max_time,
                               int bound,
                               const shared_ptr<AbstractTask> &task,
                               std::unique_ptr<ComponentMap> &&_component_map,
                               vector<shared_ptr<TaskIndependentSearchAlgorithm>> search_algorithms,
                               bool pass_bound,
                               bool repeat_last_phase,
                               bool continue_on_fail,
                               bool continue_on_solve,
                               string unparsed_config
                               ) : SearchAlgorithm(verbosity,
                                                cost_type,
                                                max_time,
                                                bound,
                                                unparsed_config,
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
    component_map = (move(_component_map));
    component_map->add_dual_key_entry(nullptr, nullptr, nullptr);
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
            log << "Starting search: " << search_algorithms[search_algorithms.size() - 1]->get_description() << endl;
            return search_algorithms[search_algorithms.size() - 1]->create_task_specific(task, component_map, 1);
        } else {
            return nullptr;
        }
    }
    log << "Starting search: " << search_algorithms[phase]->get_description() << endl;
    return search_algorithms[phase]->create_task_specific(task, component_map, 1);
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


TaskIndependentIteratedSearch::TaskIndependentIteratedSearch(utils::Verbosity verbosity,
                                                             OperatorCost cost_type,
                                                             double max_time,
                                                             int bound,
                                                             string unparsed_config,
                                                             vector<shared_ptr<TaskIndependentSearchAlgorithm>> search_algorithms,
                                                             bool pass_bound,
                                                             bool repeat_last_phase,
                                                             bool continue_on_fail,
                                                             bool continue_on_solve
                                                             )
    : TaskIndependentSearchAlgorithm(verbosity,
                                  cost_type,
                                  max_time,
                                  bound,
                                  unparsed_config),
      search_algorithms(search_algorithms),
      pass_bound(pass_bound),
      repeat_last_phase(repeat_last_phase),
      continue_on_fail(continue_on_fail),
      continue_on_solve(continue_on_solve) {
}

TaskIndependentIteratedSearch::~TaskIndependentIteratedSearch() {
}


shared_ptr<IteratedSearch> TaskIndependentIteratedSearch::create_task_specific_IteratedSearch(const shared_ptr<AbstractTask> &task, std::unique_ptr<ComponentMap> &&component_map, int depth) {
    shared_ptr<IteratedSearch> task_specific_x;
    if (component_map->contains_key(make_pair(task, static_cast<void *>(this)))) {
        cerr << "Tries to reuse task specific IteratedSearch... This should not happen" << endl;
        utils::exit_with(utils::ExitCode::SEARCH_INPUT_ERROR);
    } else {
        utils::g_log << std::string(depth, ' ') << "Creating task specific IteratedSearch..." << endl;
        task_specific_x = make_shared<IteratedSearch>(verbosity,
                                                      cost_type,
                                                      max_time,
                                                      bound,
                                                      task,
                                                      move(component_map),
                                                      search_algorithms,
                                                      pass_bound,
                                                      repeat_last_phase,
                                                      continue_on_fail,
                                                      continue_on_solve);
        utils::g_log << "Created task specific IteratedSearch..." << endl;
    }
    return task_specific_x;
}




shared_ptr<SearchAlgorithm> TaskIndependentIteratedSearch::create_task_specific_root(const shared_ptr<AbstractTask> &task, int depth) {
    utils::g_log << std::string(depth, ' ') << "Creating IteratedSearch as root component..." << endl;
    std::unique_ptr<ComponentMap> component_map = std::make_unique<ComponentMap>();
    return create_task_specific_IteratedSearch(task, move(component_map), depth);
}


shared_ptr<SearchAlgorithm> TaskIndependentIteratedSearch::create_task_specific(const shared_ptr<AbstractTask> &task, unique_ptr<ComponentMap> &component_map, int depth) {
    return create_task_specific_IteratedSearch(task, move(component_map), depth);
}

class TaskIndependentIteratedSearchFeature : public plugins::TypedFeature<TaskIndependentSearchAlgorithm, TaskIndependentIteratedSearch> {
public:
    TaskIndependentIteratedSearchFeature() : TypedFeature("iterated") {
        document_title("Iterated search");
        document_synopsis("");

        add_list_option<shared_ptr<TaskIndependentSearchAlgorithm>>(
            "search_algorithms",
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

    virtual shared_ptr<TaskIndependentIteratedSearch> create_component(const plugins::Options &opts, const utils::Context &context) const override {
        plugins::verify_list_non_empty<shared_ptr<TaskIndependentSearchEngine>>(context, opts, "engines");
        return make_shared<TaskIndependentIteratedSearch>(opts.get<utils::Verbosity>("verbosity"),
                                                          opts.get<OperatorCost>("cost_type"),
                                                          opts.get<double>("max_time"),
                                                          opts.get<int>("bound"),
                                                          opts.get_unparsed_config(),
                                                          opts.get_list<shared_ptr<TaskIndependentSearchAlgorithm>>("search_algorithms"),
                                                          opts.get<bool>("pass_bound"),
                                                          opts.get<bool>("repeat_last"),
                                                          opts.get<bool>("continue_on_fail"),
                                                          opts.get<bool>("continue_on_solve")
                                                          );
    }
};


static plugins::FeaturePlugin<TaskIndependentIteratedSearchFeature> _plugin;
}
