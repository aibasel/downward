#include "iterated_search.h"
#include <limits>

IteratedSearch::IteratedSearch(
    const vector<string> &engine_config_,
    vector<int> engine_config_start_,
    bool pass_bound_,
    bool repeat_last_phase_,
    bool continue_on_fail_,
    bool continue_on_solve_)
    : engine_config(engine_config_),
      engine_config_start(engine_config_start_),
      pass_bound(pass_bound_),
      repeat_last_phase(repeat_last_phase_),
      continue_on_fail(continue_on_fail_),
      continue_on_solve(continue_on_solve_) {
    last_phase_found_solution = false;
    best_bound = numeric_limits<int>::max();
    found_solution = false;
    plan_counter = 0;
}

IteratedSearch::~IteratedSearch() {
}

void IteratedSearch::initialize() {
    phase = 0;
}

SearchEngine *IteratedSearch::get_search_engine(
    int engine_config_start_index) {
    int end;
    current_search_name = "";
    SearchEngine *engine = OptionParser::instance()->parse_search_engine(
        engine_config, engine_config_start[engine_config_start_index],
        end, false);
    for (int i = engine_config_start[engine_config_start_index]; i <= end; i++) {
        current_search_name.append(engine_config[i]);
    }
    cout << "Starting search: " << current_search_name << endl;

    return engine;
}

SearchEngine *IteratedSearch::create_phase(int p) {
    if (p >= engine_config_start.size()) {
        if (repeat_last_phase) {
            return get_search_engine(engine_config_start.size() - 1);
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
        plan_cost = calculate_plan_cost(found_plan);
        if (plan_cost < best_bound) {
            ++plan_counter;
            plan_cost = save_plan(found_plan, plan_counter);
            best_bound = plan_cost;
            set_plan(found_plan);
        }
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



SearchEngine *IteratedSearch::create(
    const vector<string> &config, int start, int &end, bool dry_run) {
    if (config[start + 1] != "(")
        throw ParseError(start + 1);

    vector<int> engine_config_start;
    OptionParser::instance()->parse_search_engine_list(config, start + 2,
                                                       end, false, engine_config_start, true);

    if (engine_config_start.empty())
        throw ParseError(end);
    end++;

    bool pass_bound = true;
    bool repeat_last = false;
    bool continue_on_fail = false;
    bool continue_on_solve = true;

    if (config[end] != ")") {
        end++;
        NamedOptionParser option_parser;
        option_parser.add_bool_option("pass_bound", &pass_bound,
                                      "use bound from previous search");
        option_parser.add_bool_option("repeat_last", &repeat_last,
                                      "repeat last phase of search");
        option_parser.add_bool_option("continue_on_fail", &continue_on_fail,
                                      "continue search after no solution found");
        option_parser.add_bool_option("continue_on_solve", &continue_on_solve,
                                      "continue search after solution found");
        option_parser.parse_options(config, end, end, dry_run);
        end++;
    }
    if (config[end] != ")")
        throw ParseError(end);

    if (dry_run)
        return NULL;

    IteratedSearch *engine = \
        new IteratedSearch(config, engine_config_start, pass_bound, repeat_last,
                           continue_on_fail, continue_on_solve);

    return engine;
}

void IteratedSearch::save_plan_if_necessary() const {
}
