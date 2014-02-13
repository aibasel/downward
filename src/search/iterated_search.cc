#include "iterated_search.h"
#include "plugin.h"
#include "ext/tree_util.hh"
#include <limits>

IteratedSearch::IteratedSearch(const Options &opts)
    : SearchEngine(opts),
      engine_configs(opts.get_list<ParseTree>("engine_configs")),
      pass_bound(opts.get<bool>("pass_bound")),
      repeat_last_phase(opts.get<bool>("repeat_last")),
      continue_on_fail(opts.get<bool>("continue_on_fail")),
      continue_on_solve(opts.get<bool>("continue_on_solve")) {
    last_phase_found_solution = false;
    best_bound = bound;
    iterated_found_solution = false;
    plan_counter = opts.get<int>("plan_counter");
}

IteratedSearch::~IteratedSearch() {
}

void IteratedSearch::initialize() {
    phase = 0;
}

SearchEngine *IteratedSearch::get_search_engine(
    int engine_configs_index) {
    OptionParser parser(engine_configs[engine_configs_index], false);
    SearchEngine *engine = parser.start_parsing<SearchEngine *>();

    cout << "Starting search: ";
    kptree::print_tree_bracketed(engine_configs[engine_configs_index], cout);
    cout << endl;

    return engine;
}

SearchEngine *IteratedSearch::create_phase(int p) {
    if (p >= engine_configs.size()) {
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
            return NULL;
        }
    }

    return get_search_engine(p);
}

int IteratedSearch::step() {
    current_search = create_phase(phase);
    if (current_search == NULL) {
        return found_solution() ? SOLVED : FAILED;
    }
    if (pass_bound) {
        current_search->set_bound(best_bound);
    }
    phase++;

    current_search->search();

    SearchEngine::Plan found_plan;
    int plan_cost = 0;
    last_phase_found_solution = current_search->found_solution();
    if (last_phase_found_solution) {
        iterated_found_solution = true;
        found_plan = current_search->get_plan();
        plan_cost = calculate_plan_cost(found_plan);
        if (plan_cost < best_bound) {
            ++plan_counter;
            save_plan(found_plan, plan_counter);
            best_bound = plan_cost;
            set_plan(found_plan);
        }
    }
    current_search->statistics();
    search_progress.inc_expanded(
        current_search->get_search_progress().get_expanded());
    search_progress.inc_evaluated_states(
        current_search->get_search_progress().get_evaluated_states());
    search_progress.inc_evaluations(
        current_search->get_search_progress().get_evaluations());
    search_progress.inc_generated(
        current_search->get_search_progress().get_generated());
    search_progress.inc_generated_ops(
        current_search->get_search_progress().get_generated_ops());
    search_progress.inc_reopened(
        current_search->get_search_progress().get_reopened());

    return step_return_value();
}

int IteratedSearch::step_return_value() {
    if (iterated_found_solution)
        cout << "Best solution cost so far: " << best_bound << endl;

    if (last_phase_found_solution) {
        if (continue_on_solve) {
            cout << "Solution found - keep searching" << endl;
            return IN_PROGRESS;
        } else {
            cout << "Solution found - stop searching" << endl;
            return SOLVED;
        }
    } else {
        if (continue_on_fail) {
            cout << "No solution found - keep searching" << endl;
            return IN_PROGRESS;
        } else {
            cout << "No solution found - stop searching" << endl;
            return iterated_found_solution ? SOLVED : FAILED;
        }
    }
}

void IteratedSearch::statistics() const {
    cout << "Cumulative statistics:" << endl;
    search_progress.print_statistics();
}

void IteratedSearch::save_plan_if_necessary() const {
    // Don't need to save here, as we automatically save after
    // each successful search iteration.
}

static SearchEngine *_parse(OptionParser &parser) {
    parser.document_synopsis("Iterated search", "");
    parser.document_note(
        "Note 1",
        "We do no cache values between search iterations at the moment. "
        "If you perform a LAMA-style iterative search, heuristic values "
        "will be computed multiple times. "
        "Adding heuristic caching is [issue108 http://issues.fast-downward.org/issue108].");
    parser.document_note(
        "Note 2",
        "Running this\n```\n"
        "./downward --search \"iterated([lazy_wastar(merge_and_shrink(),w=10), "
        "lazy_wastar(merge_and_shrink(),w=5), lazy_wastar(merge_and_shrink(),w=3), "
        "lazy_wastar(merge_and_shrink(),w=2), lazy_wastar(merge_and_shrink(),w=1)])\"\n"
        "```\nwould perform the preprocessing phase of the merge and shrink heuristic "
        "5 times (once before each iteration).\n\n"
        "To avoid this, use heuristic predefinition, which avoids duplicate "
        "preprocessing, as follows:\n```\n"
        "./downward --heuristic \"h=merge_and_shrink()\" --search "
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
    parser.add_option<int>("plan_counter",
                           "start enumerating plans with plan_counter + 1",
                           "0");
    SearchEngine::add_options_to_parser(parser);
    Options opts = parser.parse();

    opts.verify_list_non_empty<ParseTree>("engine_configs");

    if (parser.help_mode()) {
        return 0;
    } else if (parser.dry_run()) {
        //check if the supplied search engines can be parsed
        vector<ParseTree> configs = opts.get_list<ParseTree>("engine_configs");
        for (size_t i(0); i != configs.size(); ++i) {
            OptionParser test_parser(configs[i], true);
            test_parser.start_parsing<SearchEngine *>();
        }
        return 0;
    } else {
        IteratedSearch *engine = new IteratedSearch(opts);

        return engine;
    }
}

static Plugin<SearchEngine> _plugin("iterated", _parse);
