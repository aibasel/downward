#include "iterated_search.h"

IteratedSearch::IteratedSearch(
        vector<SearchEngine *> engine_list,
        bool pass_bound_,
        bool repeat_last_phase_):
        engines(engine_list),
        pass_bound(pass_bound_),
        repeat_last_phase(repeat_last_phase_)
{
    last_phase_plan_cost = -1;
}

IteratedSearch::~IteratedSearch() {
}

void IteratedSearch::initialize() {
    phase = 0;
}

SearchEngine *IteratedSearch::create_phase(int p) {
    if (p >= engines.size()) {
        if (repeat_last_phase) {
            return engines[engines.size() - 1];
        }
        else {
            return NULL;
        }
    }
    return engines[p];
}

int IteratedSearch::step() {
    current_search = create_phase(phase);
    if (current_search == NULL) {
        return ( (last_phase_plan_cost >= 0) ? SOLVED : FAILED);
    }
    if (pass_bound && (last_phase_plan_cost >= 0)) {
        current_search->set_bound(last_phase_plan_cost);
    }
    phase++;

    current_search->search();

    SearchEngine::Plan found_plan;
    int plan_cost = 0;
    bool found_solution = current_search->found_solution();
    phase_found_solution.push_back(found_solution);
    phase_statistics.push_back(current_search->get_search_progress());
    if (found_solution) {
        found_plan = current_search->get_plan();
        plan_cost = save_plan(found_plan);
        last_phase_plan_cost = plan_cost;
        set_plan(found_plan);
    }
    else {
        last_phase_plan_cost = -1;
    }
    current_search->statistics();
    search_progress.inc_expanded(
            current_search->get_search_progress().get_expanded());
    search_progress.inc_evaluated(
            current_search->get_search_progress().get_evaluated());
    search_progress.inc_generated(
            current_search->get_search_progress().get_generated());
    search_progress.inc_generated_ops(
            current_search->get_search_progress().get_generated_ops());
    search_progress.inc_reopened(
            current_search->get_search_progress().get_reopened());

    phase_solution_cost.push_back(plan_cost);
    return step_return_value();
}

int IteratedSearch::step_return_value() {
    cout << "Last solution: " << last_phase_plan_cost << endl;
    if (last_phase_plan_cost > -1) {
        cout << "Solution found - keep searching" << endl;
        return IN_PROGRESS;
    } else {
        if (phase > 0) {
            cout << "No solution found - last solution is optimal" << endl;
            return SOLVED;
        }
        else {
            cout << "No solution found" << endl;
            return FAILED;
        }
    }
}

void IteratedSearch::statistics() const {
    int expanded = 0;
    int generated = 0;
    int reopened = 0;
    int evaluated = 0;

    for (int i = 0; i < phase_statistics.size(); i++) {
        cout << "Phase " << i << endl;
        phase_statistics[i].print_statistics();

        expanded += phase_statistics[i].get_expanded();
        generated += phase_statistics[i].get_generated();
        reopened += phase_statistics[i].get_reopened();
        evaluated += phase_statistics[i].get_evaluated();
    }

    cout << "Total" << endl;
    search_progress.print_statistics();
}



SearchEngine *IteratedSearch::create(
    const vector<string> &config, int start, int &end) {

    if (config[start + 1] != "(")
        throw ParseError(start + 1);

    vector<SearchEngine *> engines;
    OptionParser::instance()->parse_search_engine_list(config, start + 2,
                                                          end, false, engines);

    if (engines.empty())
        throw ParseError(end);
    end ++;

    bool pass_bound = true;
    bool repeat_last = false;

    if (config[end] != ")") {
        end ++;
        NamedOptionParser option_parser;
        option_parser.add_bool_option("pass_bound", &pass_bound,
                                     "use bound from previous search");
        option_parser.add_bool_option("repeat_last", &repeat_last,
                                     "repeat last phase of search");
        option_parser.parse_options(config, end, end);
        end ++;
    }
    if (config[end] != ")")
        throw ParseError(end);

    IteratedSearch *engine = \
        new IteratedSearch(engines, pass_bound, repeat_last);
    return engine;
}
