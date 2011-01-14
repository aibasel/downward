#include "best_first_search.h"

#include "globals.h"
#include "heuristic.h"
#include "successor_generator.h"
#include "open_lists/standard_scalar_open_list.h"
#include "open_lists/open_list_buckets.h"


#include <cassert>
using namespace std;

OpenListInfo::OpenListInfo(Heuristic *heur, bool only_pref)
    : open(new BucketOpenList<OpenListEntry>(heur, false)) {
    heuristic = heur;
    only_preferred_operators = only_pref;
    priority = 0;
}

BestFirstSearchEngine::BestFirstSearchEngine(const SearchEngineOptions &options)
    : SearchEngine(options), current_state(*g_initial_state) {
    generated_states = 0;
    current_predecessor = 0;
    current_operator = 0;
}

BestFirstSearchEngine::~BestFirstSearchEngine() {
}

void BestFirstSearchEngine::add_heuristic(Heuristic *heuristic,
                                          bool use_estimates,
                                          bool use_preferred_operators) {
    assert(use_estimates || use_preferred_operators);
    heuristics.push_back(heuristic);
    best_heuristic_values.push_back(-1);
    if (use_estimates) {
        open_lists.push_back(OpenListInfo(heuristic, false));
        open_lists.push_back(OpenListInfo(heuristic, true));
    }
    if (use_preferred_operators)
        preferred_operator_heuristics.push_back(heuristic);
}

void BestFirstSearchEngine::initialize() {
    cout << "Conducting best first search." << endl;
    assert(!open_lists.empty());
}

void BestFirstSearchEngine::statistics() const {
    cout << "Expanded " << closed_list.size() << " state(s)." << endl;
    cout << "Generated " << generated_states << " state(s)." << endl;
}

int BestFirstSearchEngine::step() {
    // Invariants:
    // - current_state is the next state for which we want to compute the heuristic.
    // - current_predecessor is a permanent pointer to the predecessor of that state.
    // - current_operator is the operator which leads to current_state from predecessor.

    if (!closed_list.contains(current_state)) {
        const State *parent_ptr = closed_list.insert(
            current_state, current_predecessor, current_operator);
        for (int i = 0; i < heuristics.size(); i++) {
            if (current_predecessor != NULL)
                heuristics[i]->reach_state(*current_predecessor, *current_operator, *parent_ptr);
            heuristics[i]->evaluate(current_state);
        }
        if (!is_dead_end()) {
            if (check_goal())
                return SOLVED;
            if (check_progress()) {
                report_progress();
                reward_progress();
            }
            generate_successors(parent_ptr);
        }
    }
    return fetch_next_state();
}

bool BestFirstSearchEngine::is_dead_end() {
    // If a reliable heuristic reports a dead end, we trust it.
    // Otherwise, all heuristics must agree on dead-end-ness.
    int dead_end_counter = 0;
    for (int i = 0; i < heuristics.size(); i++) {
        if (heuristics[i]->is_dead_end()) {
            if (heuristics[i]->dead_ends_are_reliable())
                return true;
            else
                dead_end_counter++;
        }
    }
    return dead_end_counter == heuristics.size();
}

bool BestFirstSearchEngine::check_goal() {
    // Any heuristic reports 0 iff this is a goal state, so we can
    // pick an arbitrary one. (only if there are no action costs)
    Heuristic *heur = open_lists[0].heuristic;
    if (!heur->is_dead_end() && heur->get_heuristic() == 0) {
        // We actually need this silly !heur->is_dead_end() check because
        // this state *might* be considered a non-dead end by the
        // overall search even though heur considers it a dead end
        // (e.g. if heur is the CG heuristic, but the FF heuristic is
        // also computed and doesn't consider this state a dead end.
        // If heur considers the state a dead end, it cannot be a goal
        // state (heur will not be *that* stupid). We may not call
        // get_heuristic() in such cases because it will barf.
        if (g_use_metric) {
            bool is_goal = test_goal(current_state);
            if (!is_goal)
                return false;
        }

        cout << "Solution found!" << endl;
        Plan plan;
        closed_list.trace_path(current_state, plan);
        set_plan(plan);
        return true;
    } else {
        return false;
    }
}

bool BestFirstSearchEngine::check_progress() {
    bool progress = false;
    for (int i = 0; i < heuristics.size(); i++) {
        if (heuristics[i]->is_dead_end())
            continue;
        int h = heuristics[i]->get_heuristic();
        int &best_h = best_heuristic_values[i];
        if (best_h == -1 || h < best_h) {
            best_h = h;
            progress = true;
        }
    }
    return progress;
}

void BestFirstSearchEngine::report_progress() {
    cout << "Best heuristic value: ";
    for (int i = 0; i < heuristics.size(); i++) {
        cout << best_heuristic_values[i];
        if (i != heuristics.size() - 1)
            cout << "/";
    }
    cout << " [expanded " << closed_list.size() << " state(s)]" << endl;
}

void BestFirstSearchEngine::reward_progress() {
    // Boost the "preferred operator" open lists somewhat whenever
    // progress is made. This used to be used in multi-heuristic mode
    // only, but it is also useful in single-heuristic mode, at least
    // in Schedule.
    //
    // TODO: Test the impact of this, and find a better way of rewarding
    // successful exploration. For example, reward only the open queue
    // from which the good state was extracted and/or the open queues
    // for the heuristic for which a new best value was found.

    for (int i = 0; i < open_lists.size(); i++)
        if (open_lists[i].only_preferred_operators)
            open_lists[i].priority -= 1000;
}

void BestFirstSearchEngine::generate_successors(const State *parent_ptr) {
    vector<const Operator *> all_operators;
    g_successor_generator->generate_applicable_ops(current_state, all_operators);

    vector<const Operator *> preferred_operators;
    for (int i = 0; i < preferred_operator_heuristics.size(); i++) {
        Heuristic *heur = preferred_operator_heuristics[i];
        if (!heur->is_dead_end())
            heur->get_preferred_operators(preferred_operators);
    }

    for (int i = 0; i < open_lists.size(); i++) {
        Heuristic *heur = open_lists[i].heuristic;
        if (!heur->is_dead_end()) {
            OpenList<OpenListEntry> *open = open_lists[i].open;
            open->evaluate(0, false); // TODO: handle preferredness in open list
            vector<const Operator *> &ops =
                open_lists[i].only_preferred_operators ?
                preferred_operators : all_operators;
            for (int j = 0; j < ops.size(); j++)
                open->insert(make_pair(parent_ptr, ops[j]));
        }
    }
    generated_states += all_operators.size();
}

int BestFirstSearchEngine::fetch_next_state() {
    OpenListInfo *open_info = select_open_queue();
    if (!open_info) {
        cout << "Completely explored state space -- no solution!" << endl;
        return FAILED;
    }

    OpenListEntry next = open_info->open->remove_min();
    open_info->priority++;

    current_predecessor = next.first;
    current_operator = next.second;
    current_state = State(*current_predecessor, *current_operator);

    return IN_PROGRESS;
}

SearchEngine *BestFirstSearchEngine::create(const vector<string> &config,
                                            int start, int &end, bool dry_run) {
    if (config[start + 1] != "(")
        throw ParseError(start + 1);

    SearchEngineOptions common_options;
    vector<Heuristic *> evals;
    OptionParser::instance()->parse_heuristic_list(config, start + 2,
                                                   end, false, evals, dry_run);
    if (evals.empty())
        throw ParseError(end);
    end++;

    vector<Heuristic *> preferred_list;

    if (config[end] != ")") {
        end++;
        NamedOptionParser option_parser;
        common_options.add_options_to_parser(option_parser);

        option_parser.add_heuristic_list_option("preferred",
                                                &preferred_list, "use preferred operators of these heuristics");
        option_parser.parse_options(config, end, end, dry_run);
        end++;
    }
    if (config[end] != ")")
        throw ParseError(end);

    BestFirstSearchEngine *engine = 0;
    if (!dry_run) {
        engine = new BestFirstSearchEngine(common_options);

        set<Heuristic *> hset;
        set<Heuristic *> pset;
        hset.insert(evals.begin(), evals.end());
        pset.insert(preferred_list.begin(), preferred_list.end());

        for (unsigned int i = 0; i < evals.size(); i++) {
            engine->add_heuristic(evals[i], true, pset.count(evals[i]));
        }
        for (unsigned int i = 0; i < preferred_list.size(); i++) {
            if (hset.count(preferred_list[0]) == 0) {
                engine->add_heuristic(evals[i], false, true);
            }
        }
    }

    return engine;
}

OpenListInfo *BestFirstSearchEngine::select_open_queue() {
    OpenListInfo *best = 0;
    for (int i = 0; i < open_lists.size(); i++)
        if (!open_lists[i].open->empty() &&
            (best == 0 || open_lists[i].priority < best->priority))
            best = &open_lists[i];
    return best;
}
