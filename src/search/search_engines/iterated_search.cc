#include "iterated_search.h"

#include "../option_parser.h"
#include "../plugin.h"

#include "../utils/logging.h"

#include <iostream>

using namespace std;

namespace iterated_search {
IteratedSearch::IteratedSearch(const Options &opts, options::Registry &registry,
                               const options::Predefinitions &predefinitions)
    : SearchEngine(opts),
      engine_configs(opts.get_list<ParseTree>("engine_configs")),
      registry(registry),
      predefinitions(predefinitions),
      pass_bound(opts.get<bool>("pass_bound")),
      repeat_last_phase(opts.get<bool>("repeat_last")),
      continue_on_fail(opts.get<bool>("continue_on_fail")),
      continue_on_solve(opts.get<bool>("continue_on_solve")),
      phase(0),
      last_phase_found_solution(false),
      best_bound(bound),
      iterated_found_solution(false) {
}

shared_ptr<SearchEngine> IteratedSearch::get_search_engine(
    int engine_configs_index) {
    OptionParser parser(engine_configs[engine_configs_index], registry, predefinitions, false);
    shared_ptr<SearchEngine> engine(parser.start_parsing<shared_ptr<SearchEngine>>());

    ostringstream stream;
    kptree::print_tree_bracketed(engine_configs[engine_configs_index], stream);
    utils::g_log << "Starting search: " << stream.str() << endl;

    return engine;
}

shared_ptr<SearchEngine> IteratedSearch::create_current_phase() {
    int num_phases = engine_configs.size();
    if (phase >= num_phases) {
        /* We've gone through all searches. We continue if
           repeat_last_phase is true, but *not* if we didn't find a
           solution the last time around, since then this search would
           just behave the same way again (assuming determinism, which
           we might not actually have right now, but strive for). So
           this overrides continue_on_fail.
        */
        if (repeat_last_phase && last_phase_found_solution) {
            return get_search_engine(engine_configs.size() - 1);
        } else {
            return nullptr;
        }
    }

    return get_search_engine(phase);
}

SearchStatus IteratedSearch::step() {
    shared_ptr<SearchEngine> current_search = create_current_phase();
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
        utils::g_log << "Best solution cost so far: " << best_bound << endl;

    if (last_phase_found_solution) {
        if (continue_on_solve) {
            utils::g_log << "Solution found - keep searching" << endl;
            return IN_PROGRESS;
        } else {
            utils::g_log << "Solution found - stop searching" << endl;
            return SOLVED;
        }
    } else {
        if (continue_on_fail) {
            utils::g_log << "No solution found - keep searching" << endl;
            return IN_PROGRESS;
        } else {
            utils::g_log << "No solution found - stop searching" << endl;
            return iterated_found_solution ? SOLVED : FAILED;
        }
    }
}

void IteratedSearch::print_statistics() const {
    utils::g_log << "Cumulative statistics:" << endl;
    statistics.print_detailed_statistics();
}

void IteratedSearch::save_plan_if_necessary() {
    // We don't need to save here, as we automatically save after
    // each successful search iteration.
}

static shared_ptr<SearchEngine> _parse(OptionParser &parser) {
    parser.document_synopsis("Iterated search", "");
    parser.document_note(
        "Note 1",
        "We don't cache heuristic values between search iterations at"
        " the moment. If you perform a LAMA-style iterative search,"
        " heuristic values will be computed multiple times.");
    parser.document_note(
        "Note 2",
        "The configuration\n```\n"
        "--search \"iterated([lazy_wastar(merge_and_shrink(),w=10), "
        "lazy_wastar(merge_and_shrink(),w=5), lazy_wastar(merge_and_shrink(),w=3), "
        "lazy_wastar(merge_and_shrink(),w=2), lazy_wastar(merge_and_shrink(),w=1)])\"\n"
        "```\nwould perform the preprocessing phase of the merge and shrink heuristic "
        "5 times (once before each iteration).\n\n"
        "To avoid this, use heuristic predefinition, which avoids duplicate "
        "preprocessing, as follows:\n```\n"
        "--evaluator \"h=merge_and_shrink()\" --search "
        "\"iterated([lazy_wastar(h,w=10), lazy_wastar(h,w=5), lazy_wastar(h,w=3), "
        "lazy_wastar(h,w=2), lazy_wastar(h,w=1)])\"\n"
        "```");
    parser.document_note(
        "Note 3",
        "If you reuse the same landmark count heuristic "
        "(using heuristic predefinition) between iterations, "
        "the path data (that is, landmark status for each visited state) "
        "will be saved between iterations.");
    parser.add_list_option<ParseTree>("engine_configs",
                                      "list of search engines for each phase");
    parser.add_option<bool>(
        "pass_bound",
        "use bound from previous search. The bound is the real cost "
        "of the plan found before, regardless of the cost_type parameter.",
        "true");
    parser.add_option<bool>("repeat_last",
                            "repeat last phase of search",
                            "false");
    parser.add_option<bool>("continue_on_fail",
                            "continue search after no solution found",
                            "false");
    parser.add_option<bool>("continue_on_solve",
                            "continue search after solution found",
                            "true");
    SearchEngine::add_options_to_parser(parser);
    Options opts = parser.parse();

    opts.verify_list_non_empty<ParseTree>("engine_configs");

    if (parser.help_mode()) {
        return nullptr;
    } else if (parser.dry_run()) {
        //check if the supplied search engines can be parsed
        for (const ParseTree &config : opts.get_list<ParseTree>("engine_configs")) {
            OptionParser test_parser(config, parser.get_registry(),
                                     parser.get_predefinitions(), true);
            test_parser.start_parsing<shared_ptr<SearchEngine>>();
        }
        return nullptr;
    } else {
        return make_shared<IteratedSearch>(opts, parser.get_registry(),
                                           parser.get_predefinitions());
    }
}

static Plugin<SearchEngine> _plugin("iterated", _parse);
}
