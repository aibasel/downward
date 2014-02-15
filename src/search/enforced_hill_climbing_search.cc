#include "enforced_hill_climbing_search.h"
#include "successor_generator.h"
#include "heuristic.h"
#include "operator.h"
#include "pref_evaluator.h"
#include "plugin.h"
#include "utilities.h"

EnforcedHillClimbingSearch::EnforcedHillClimbingSearch(
    const Options &opts)
    : SearchEngine(opts),
      heuristic(opts.get<Heuristic *>("h")),
      use_preferred(false),
      preferred_usage(PreferredUsage(opts.get_enum("preferred_usage"))),
      current_state(g_initial_state()),
      num_ehc_phases(0) {
    if (opts.contains("preferred")) {
        preferred_heuristics = opts.get_list<Heuristic *>("preferred");
        if (preferred_heuristics.empty()) {
            use_preferred = false;
            preferred_contains_eval = false;
        } else if (find(preferred_heuristics.begin(),
                        preferred_heuristics.end(),
                        heuristic) != preferred_heuristics.end()) {
            use_preferred = true;
            preferred_contains_eval = true;
        }
    }
    search_progress.add_heuristic(heuristic);
    g_evaluator = new GEvaluator();
}

EnforcedHillClimbingSearch::~EnforcedHillClimbingSearch() {
    delete g_evaluator;
}

void EnforcedHillClimbingSearch::evaluate(const State &parent, const Operator *op, const State &state) {
    search_progress.inc_evaluated_states();

    if (!preferred_contains_eval) {
        if (op != NULL) {
            heuristic->reach_state(parent, *op, state);
        }
        heuristic->evaluate(state);
        search_progress.inc_evaluations();
    }
    for (int i = 0; i < preferred_heuristics.size(); i++) {
        if (op != NULL) {
            preferred_heuristics[i]->reach_state(parent, *op, state);
        }
        preferred_heuristics[i]->evaluate(state);
    }
    search_progress.inc_evaluations(preferred_heuristics.size());
}

void EnforcedHillClimbingSearch::initialize() {
    assert(heuristic != NULL);
    current_g = 0;
    cout << "Conducting Enforced Hill Climbing Search" << endl;
    if (use_preferred) {
        cout << "Using preferred operators for "
             << (preferred_usage == RANK_PREFERRED_FIRST ? "ranking successors"
            : "pruning") << endl;
    }
    cout << "(real) g-bound = " << bound << endl;

    SearchNode node = search_space.get_node(current_state);
    evaluate(current_state, NULL, current_state);

    if (heuristic->is_dead_end()) {
        cout << "Initial state is a dead end, no solution" << endl;
        if (heuristic->dead_ends_are_reliable())
            exit_with(EXIT_UNSOLVABLE);
        else
            exit_with(EXIT_UNSOLVED_INCOMPLETE);
    }

    search_progress.get_initial_h_values();

    current_h = heuristic->get_heuristic();
    node.open_initial(current_h);

    if (!use_preferred || (preferred_usage == PRUNE_BY_PREFERRED)) {
        open_list = new StandardScalarOpenList<OpenListEntryEHC>(g_evaluator, false);
    } else {
        vector<ScalarEvaluator *> evals;
        evals.push_back(g_evaluator);
        evals.push_back(new PrefEvaluator);
        open_list = new TieBreakingOpenList<OpenListEntryEHC>(evals, false, true);
    }
}

void EnforcedHillClimbingSearch::get_successors(const State &state, vector<const Operator *> &ops) {
    if (!use_preferred || preferred_usage == RANK_PREFERRED_FIRST) {
        g_successor_generator->generate_applicable_ops(state, ops);

        // mark preferred operators as preferred
        if (use_preferred && (preferred_usage == RANK_PREFERRED_FIRST)) {
            for (int i = 0; i < ops.size(); i++) {
                ops[i]->unmark();
            }
            vector<const Operator *> preferred_ops;
            for (int i = 0; i < preferred_heuristics.size(); i++) {
                preferred_heuristics[i]->get_preferred_operators(preferred_ops);
            }
            for (int i = 0; i < preferred_ops.size(); i++) {
                preferred_ops[i]->mark();
            }
        }
    } else {
        vector<const Operator *> preferred_ops;
        for (int i = 0; i < preferred_heuristics.size(); i++) {
            preferred_heuristics[i]->get_preferred_operators(preferred_ops);
            for (int j = 0; j < preferred_ops.size(); j++) {
                if (!preferred_ops[j]->is_marked()) {
                    preferred_ops[j]->mark();
                    ops.push_back(preferred_ops[j]);
                }
            }
        }
    }
    search_progress.inc_expanded();
    search_progress.inc_generated_ops(ops.size());
}

int EnforcedHillClimbingSearch::step() {
    //cout << "s = ";
    //for (int i = 0; i < g_variable_domain.size(); i++) {
    //    cout << current_state[i] << " ";
    //}
    //cout << endl;
    last_expanded = search_progress.get_expanded();
    search_progress.check_h_progress(current_g);

    // current_state is the current state, and it is the last state to be evaluated
    // current_h is the h value of the current state

    if (check_goal_and_set_plan(current_state)) {
        return SOLVED;
    }

    vector<const Operator *> ops;
    get_successors(current_state, ops);

    SearchNode current_node = search_space.get_node(current_state);
    current_node.close();

    for (int i = 0; i < ops.size(); i++) {
        int d = get_adjusted_cost(*ops[i]);
        OpenListEntryEHC entry = make_pair(current_state.get_id(), make_pair(d, ops[i]));
        open_list->evaluate(d, ops[i]->is_marked());
        open_list->insert(entry);
        ops[i]->unmark();
    }
    return ehc();
}

int EnforcedHillClimbingSearch::ehc() {
    while (!open_list->empty()) {
        OpenListEntryEHC next = open_list->remove_min();
        StateID last_parent_id = next.first;
        State last_parent = g_state_registry->lookup_state(last_parent_id);
        int d = next.second.first;
        const Operator *last_op = next.second.second;

        if (search_space.get_node(last_parent).get_real_g() + last_op->get_cost() >= bound)
            continue;

        State s = g_state_registry->get_successor_state(last_parent, *last_op);
        search_progress.inc_generated();

        SearchNode node = search_space.get_node(s);

        if (node.is_new()) {
            evaluate(last_parent, last_op, s);

            if (heuristic->is_dead_end()) {
                node.mark_as_dead_end();
                search_progress.inc_dead_ends();
                continue;
            }

            int h = heuristic->get_heuristic();
            node.open(h, search_space.get_node(last_parent), last_op);

            if (h < current_h) {
                current_g = node.get_g();
                num_ehc_phases++;
                if (d_counts.find(d) == d_counts.end()) {
                    d_counts[d] = make_pair(0, 0);
                }
                pair<int, int> p = d_counts[d];
                p.first = p.first + 1;
                p.second = p.second + search_progress.get_expanded() - last_expanded;
                d_counts[d] = p;

                current_state = s;
                current_h = heuristic->get_heuristic();
                open_list->clear();
                return IN_PROGRESS;
            } else {
                vector<const Operator *> ops;
                get_successors(s, ops);

                node.close();
                for (int i = 0; i < ops.size(); i++) {
                    int new_d = d + get_adjusted_cost(*ops[i]);
                    OpenListEntryEHC entry = make_pair(node.get_state_id(), make_pair(new_d, ops[i]));
                    open_list->evaluate(new_d, ops[i]->is_marked());
                    open_list->insert(entry);
                    ops[i]->unmark();
                }
            }
        }
    }
    cout << "No solution - FAILED" << endl;
    return FAILED;
}

void EnforcedHillClimbingSearch::statistics() const {
    search_progress.print_statistics();

    cout << "EHC Phases: " << num_ehc_phases << endl;
    cout << "Average expansions per EHC Phase: " << (double)search_progress.get_expanded() / (double)num_ehc_phases << endl;

    map<int, pair<int, int> >::const_iterator it;
    for (it = d_counts.begin(); it != d_counts.end(); it++) {
        pair<int, pair<int, int> > p = *it;
        cout << "EHC phases of depth " << p.first << " : " << p.second.first << " - Avg. Expansions: " << (double)p.second.second / (double)p.second.first << endl;
    }
}

static SearchEngine *_parse(OptionParser &parser) {
    parser.document_synopsis("Enforced hill-climbing", "");
    parser.add_option<Heuristic *>("h", "heuristic");
    parser.add_option<bool>("bfs_use_cost",
                            "use cost for bfs", "false");
    vector<string> preferred_usages;
    preferred_usages.push_back("PRUNE_BY_PREFERRED");
    preferred_usages.push_back("RANK_PREFERRED_FIRST");
    parser.add_enum_option("preferred_usage", preferred_usages,
                           "preferred operator usage",
                           "PRUNE_BY_PREFERRED");

    parser.add_list_option<Heuristic *>(
        "preferred",
        "use preferred operators of these heuristics", "[]");
    SearchEngine::add_options_to_parser(parser);
    Options opts = parser.parse();

    EnforcedHillClimbingSearch *engine = 0;
    if (!parser.dry_run()) {
        engine = new EnforcedHillClimbingSearch(opts);
    }

    return engine;
}

static Plugin<SearchEngine> _plugin("ehc", _parse);
