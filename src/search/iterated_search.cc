#include "iterated_search.h"
#include <limits>

IteratedSearch::IteratedSearch(opts)
    : engine_configs(opts.get_list<ParseTree>("engine_configs")),
      pass_bound(opts.get<bool>("pass_bound")),
      repeat_last_phase(opts.get<bool>("repeat_last_phase")),
      continue_on_fail(opts.get<bool>("continue_on_fail")),
      continue_on_solve(opts.get<bool>("continue_on_solve")) {
    last_phase_found_solution = false;
    best_bound = numeric_limits<int>::max();
    found_solution = false;
}

IteratedSearch::~IteratedSearch() {
}

void IteratedSearch::initialize() {
    phase = 0;
}

SearchEngine *IteratedSearch::get_search_engine(
    int engine_configs_index) {

    current_search_name = engine_configs[engine_configs_index].toStr();
    OptionParser parser(engine_configs[engine_configs_index], false);
    SearchEngine *engine = parser->parse_search_engine()

    cout << "Starting search: " << current_search_name << endl;

    return engine;
}

SearchEngine *IteratedSearch::create_phase(int p) {
    if (p >= engine_configs.size()) {
        if (repeat_last_phase) {
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
        return (last_phase_found_solution) ? SOLVED : FAILED;
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
        found_solution = true;
        found_plan = current_search->get_plan();
        plan_cost = save_plan(found_plan);
        if (plan_cost < best_bound)
            best_bound = plan_cost;
        set_plan(found_plan);
    }
    phase_statistics.push_back(current_search->get_search_progress());
    phase_solution_cost.push_back(plan_cost);
    phase_found_solution.push_back(last_phase_found_solution);

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
    if (found_solution)
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
            return found_solution ? SOLVED : FAILED;
        }
    }
}

void IteratedSearch::statistics() const {
    for (int i = 0; i < phase_statistics.size(); i++) {
        cout << "Phase " << i << endl;
        phase_statistics[i].print_statistics();
    }

    cout << "Total" << endl;
    search_progress.print_statistics();
}

static SearchEngine *_parse(OptionParser &parser) {
 
    parser.add_list_option<ParseTree>("engine_config", "", false);
    paser.add_option<bool>("pass_bound", true, 
                           "use bound from previous search");
    parser.add_option<bool>("repeat_last", false, 
                            "repeat last phase of search");
    parser.add_option<bool>("continue_on_fail", false,
                            "continue search after no solution found");
    parser.add_option<bool>("continue_on_solve", true,
                            "continue search after solution found");

    Options opts = parser.parse();

    if (parser.dry_run()) {
        //check if the supplied search engines can be parsed
        vector<ParseTree> configs = opts.get_list<ParseTree>("engine_config");
        for (size_t i(0); i != configs.size(); ++i) {
            Parser test_parser(configs[i], true);
            test_parser.parse_search_engine();
        }
        return 0;
    }

    IteratedSearch *engine = new IteratedSearch(opts)

    return engine;
}

static EnginePlugin _plugin("iterated", _parse);


