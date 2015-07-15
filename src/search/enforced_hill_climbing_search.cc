#include "enforced_hill_climbing_search.h"

#include "g_evaluator.h"
#include "global_operator.h"
#include "plugin.h"
#include "pref_evaluator.h"
#include "successor_generator.h"
#include "utilities.h"

using namespace std;

EnforcedHillClimbingSearch::EnforcedHillClimbingSearch(
    const Options &opts)
    : SearchEngine(opts),
      g_evaluator(new GEvaluator()),
      heuristic(opts.get<Heuristic *>("h")),
      preferred_operator_heuristics(opts.get_list<Heuristic *>("preferred")),
      preferred_usage(PreferredUsage(opts.get_enum("preferred_usage"))),
      current_eval_context(g_initial_state(), &statistics),
      num_ehc_phases(0),
      last_num_expanded(-1) {
    heuristics.insert(preferred_operator_heuristics.begin(),
                      preferred_operator_heuristics.end());
    heuristics.insert(heuristic);
    use_preferred = find(preferred_operator_heuristics.begin(),
                         preferred_operator_heuristics.end(), heuristic) !=
                    preferred_operator_heuristics.end();

    if (!use_preferred || preferred_usage == PreferredUsage::PRUNE_BY_PREFERRED) {
        open_list = new StandardScalarOpenList<OpenListEntryEHC>(g_evaluator, false);
    } else {
        vector<ScalarEvaluator *> evals {
            g_evaluator, new PrefEvaluator
        };
        open_list = new TieBreakingOpenList<OpenListEntryEHC>(evals, false, true);
    }
}

EnforcedHillClimbingSearch::~EnforcedHillClimbingSearch() {
    delete g_evaluator;
    delete open_list;
}

void EnforcedHillClimbingSearch::reach_state(
    const GlobalState &parent, const GlobalOperator &op, const GlobalState &state) {
    for (Heuristic *heur : heuristics) {
        heur->reach_state(parent, op, state);
    }
}

void EnforcedHillClimbingSearch::initialize() {
    assert(heuristic);
    cout << "Conducting enforced hill-climbing search, (real) bound = "
         << bound << endl;
    if (use_preferred) {
        cout << "Using preferred operators for "
             << (preferred_usage == PreferredUsage::RANK_PREFERRED_FIRST ?
            "ranking successors" : "pruning") << endl;
    }

    bool dead_end = current_eval_context.is_heuristic_infinite(heuristic);
    statistics.inc_evaluated_states();
    print_initial_h_values(current_eval_context);

    if (dead_end) {
        cout << "Initial state is a dead end, no solution" << endl;
        if (heuristic->dead_ends_are_reliable())
            exit_with(EXIT_UNSOLVABLE);
        else
            exit_with(EXIT_UNSOLVED_INCOMPLETE);
    }

    int current_h = current_eval_context.get_heuristic_value(heuristic);
    SearchNode node = search_space.get_node(current_eval_context.get_state());
    node.open_initial(current_h);
}

vector<const GlobalOperator *> EnforcedHillClimbingSearch::get_successors(
    EvaluationContext &eval_context) {
    vector<const GlobalOperator *> ops;
    if (!use_preferred || preferred_usage == PreferredUsage::RANK_PREFERRED_FIRST) {
        g_successor_generator->generate_applicable_ops(eval_context.get_state(), ops);

        // Mark preferred operators.
        if (use_preferred &&
            preferred_usage == PreferredUsage::RANK_PREFERRED_FIRST) {
            for (const GlobalOperator *op : ops) {
                op->unmark();
            }
            for (Heuristic *pref_heuristic : preferred_operator_heuristics) {
                const vector<const GlobalOperator *> &pref_ops =
                    eval_context.get_preferred_operators(pref_heuristic);
                for (const GlobalOperator *op : pref_ops) {
                    op->mark();
                }
            }
        }
    } else {
        for (Heuristic *pref_heuristic : preferred_operator_heuristics) {
            const vector<const GlobalOperator *> &pref_ops =
                eval_context.get_preferred_operators(pref_heuristic);
            for (const GlobalOperator *op : pref_ops) {
                if (!op->is_marked()) {
                    op->mark();
                    ops.push_back(op);
                }
            }
        }
    }
    statistics.inc_expanded();
    statistics.inc_generated_ops(ops.size());
    return ops;
}

void EnforcedHillClimbingSearch::expand(EvaluationContext &eval_context, int d) {
    for (const GlobalOperator *op : get_successors(eval_context)) {
        int new_d = d + get_adjusted_cost(*op);
        OpenListEntryEHC entry = make_pair(
            eval_context.get_state().get_id(), make_pair(new_d, op));
        EvaluationContext new_eval_context(
            eval_context.get_cache(), new_d, op->is_marked(), &statistics);
        open_list->insert(new_eval_context, entry);
        op->unmark();
    }

    SearchNode node = search_space.get_node(eval_context.get_state());
    node.close();
}

SearchStatus EnforcedHillClimbingSearch::step() {
    last_num_expanded = statistics.get_expanded();
    search_progress.check_progress(current_eval_context);

    if (check_goal_and_set_plan(current_eval_context.get_state())) {
        return SOLVED;
    }

    expand(current_eval_context, 0);
    return ehc();
}

SearchStatus EnforcedHillClimbingSearch::ehc() {
    while (!open_list->empty()) {
        OpenListEntryEHC entry = open_list->remove_min();
        StateID parent_state_id = entry.first;
        int d = entry.second.first;
        const GlobalOperator *last_op = entry.second.second;

        GlobalState parent_state = g_state_registry->lookup_state(parent_state_id);
        SearchNode parent_node = search_space.get_node(parent_state);

        if (parent_node.get_real_g() + last_op->get_cost() >= bound)
            continue;

        GlobalState state = g_state_registry->get_successor_state(parent_state, *last_op);
        statistics.inc_generated();

        SearchNode node = search_space.get_node(state);

        if (node.is_new()) {
            EvaluationContext eval_context(state, &statistics);
            reach_state(parent_state, *last_op, state);
            statistics.inc_evaluated_states();

            if (eval_context.is_heuristic_infinite(heuristic)) {
                node.mark_as_dead_end();
                statistics.inc_dead_ends();
                continue;
            }

            int h = eval_context.get_heuristic_value(heuristic);
            node.open(h, parent_node, last_op);

            if (h < current_eval_context.get_heuristic_value(heuristic)) {
                ++num_ehc_phases;
                if (d_counts.count(d) == 0) {
                    d_counts[d] = make_pair(0, 0);
                }
                pair<int, int> &d_pair = d_counts[d];
                d_pair.first += 1;
                d_pair.second += statistics.get_expanded() - last_num_expanded;

                current_eval_context = eval_context;
                open_list->clear();
                return IN_PROGRESS;
            } else {
                expand(eval_context, d);
            }
        }
    }
    cout << "No solution - FAILED" << endl;
    return FAILED;
}

void EnforcedHillClimbingSearch::print_statistics() const {
    statistics.print_detailed_statistics();

    cout << "EHC phases: " << num_ehc_phases << endl;
    assert(num_ehc_phases != 0);
    cout << "Average expansions per EHC phase: "
         << static_cast<double>(statistics.get_expanded()) / num_ehc_phases
         << endl;

    for (auto count : d_counts) {
        int depth = count.first;
        int phases = count.second.first;
        assert(phases != 0);
        int total_expansions = count.second.second;
        cout << "EHC phases of depth " << depth << ": " << phases
             << " - Avg. Expansions: "
             << static_cast<double>(total_expansions) / phases << endl;
    }
}

static SearchEngine *_parse(OptionParser &parser) {
    parser.document_synopsis("Lazy enforced hill-climbing", "");
    parser.add_option<Heuristic *>("h", "heuristic");
    vector<string> preferred_usages;
    preferred_usages.push_back("PRUNE_BY_PREFERRED");
    preferred_usages.push_back("RANK_PREFERRED_FIRST");
    parser.add_enum_option(
        "preferred_usage",
        preferred_usages,
        "preferred operator usage",
        "PRUNE_BY_PREFERRED");
    parser.add_list_option<Heuristic *>(
        "preferred",
        "use preferred operators of these heuristics",
        "[]");
    SearchEngine::add_options_to_parser(parser);
    Options opts = parser.parse();

    if (parser.dry_run())
        return nullptr;
    else
        return new EnforcedHillClimbingSearch(opts);
}

static Plugin<SearchEngine> _plugin("ehc", _parse);
