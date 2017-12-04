#include "enforced_hill_climbing_search.h"

#include "../heuristic.h"
#include "../option_parser.h"
#include "../plugin.h"

#include "../algorithms/ordered_set.h"
#include "../evaluators/g_evaluator.h"
#include "../evaluators/pref_evaluator.h"
#include "../open_lists/standard_scalar_open_list.h"
#include "../open_lists/tiebreaking_open_list.h"
#include "../task_utils/successor_generator.h"
#include "../utils/system.h"

using namespace std;
using utils::ExitCode;

namespace enforced_hill_climbing_search {
using GEval = g_evaluator::GEvaluator;
using PrefEval = pref_evaluator::PrefEvaluator;

static shared_ptr<OpenListFactory> create_ehc_open_list_factory(
    bool use_preferred, PreferredUsage preferred_usage) {
    /*
      TODO: this g-evaluator should probably be set up to always
      ignore costs since EHC is supposed to implement a breadth-first
      search, not a uniform-cost search. So this seems to be a bug.
    */
    Evaluator *g_evaluator = new GEval();

    if (!use_preferred ||
        preferred_usage == PreferredUsage::PRUNE_BY_PREFERRED) {
        /*
          TODO: Reduce code duplication with search_common.cc,
          function create_standard_scalar_open_list_factory.

          It would probably make sense to add a factory function or
          constructor that encapsulates this work to the standard
          scalar open list code.
        */
        Options options;
        options.set("eval", g_evaluator);
        options.set("pref_only", false);
        return make_shared<standard_scalar_open_list::StandardScalarOpenListFactory>(options);
    } else {
        /*
          TODO: Reduce code duplication with search_common.cc,
          function create_astar_open_list_factory_and_f_eval.

          It would probably make sense to add a factory function or
          constructor that encapsulates this work to the tie-breaking
          open list code.
        */
        vector<Evaluator *> evals = {g_evaluator, new PrefEval()};
        Options options;
        options.set("evals", evals);
        options.set("pref_only", false);
        options.set("unsafe_pruning", true);
        return make_shared<tiebreaking_open_list::TieBreakingOpenListFactory>(options);
    }
}


EnforcedHillClimbingSearch::EnforcedHillClimbingSearch(
    const Options &opts)
    : SearchEngine(opts),
      heuristic(opts.get<Heuristic *>("h")),
      preferred_operator_heuristics(opts.get_list<Heuristic *>("preferred")),
      preferred_usage(PreferredUsage(opts.get_enum("preferred_usage"))),
      current_eval_context(state_registry.get_initial_state(), &statistics),
      current_phase_start_g(-1),
      num_ehc_phases(0),
      last_num_expanded(-1) {
    heuristics.insert(preferred_operator_heuristics.begin(),
                      preferred_operator_heuristics.end());
    heuristics.insert(heuristic);
    const GlobalState &initial_state = state_registry.get_initial_state();
    for (Heuristic *heuristic : heuristics) {
        heuristic->notify_initial_state(initial_state);
    }
    use_preferred = find(preferred_operator_heuristics.begin(),
                         preferred_operator_heuristics.end(), heuristic) !=
                    preferred_operator_heuristics.end();

    open_list = create_ehc_open_list_factory(
        use_preferred, preferred_usage)->create_edge_open_list();
}

EnforcedHillClimbingSearch::~EnforcedHillClimbingSearch() {
}

void EnforcedHillClimbingSearch::reach_state(
    const GlobalState &parent, OperatorID op_id, const GlobalState &state) {
    for (Heuristic *heur : heuristics) {
        heur->notify_state_transition(parent, op_id, state);
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
            utils::exit_with(ExitCode::UNSOLVABLE);
        else
            utils::exit_with(ExitCode::UNSOLVED_INCOMPLETE);
    }

    SearchNode node = search_space.get_node(current_eval_context.get_state());
    node.open_initial();

    current_phase_start_g = 0;
}

void EnforcedHillClimbingSearch::insert_successor_into_open_list(
    const EvaluationContext &eval_context,
    int parent_g,
    OperatorID op_id,
    bool preferred) {
    OperatorProxy op = task_proxy.get_operators()[op_id];
    int succ_g = parent_g + get_adjusted_cost(op);
    EdgeOpenListEntry entry = make_pair(
        eval_context.get_state().get_id(), op_id);
    EvaluationContext new_eval_context(
        eval_context.get_cache(), succ_g, preferred, &statistics);
    open_list->insert(new_eval_context, entry);
    statistics.inc_generated_ops();
}

void EnforcedHillClimbingSearch::expand(EvaluationContext &eval_context) {
    SearchNode node = search_space.get_node(eval_context.get_state());
    int node_g = node.get_g();

    ordered_set::OrderedSet<OperatorID> preferred_operators;
    if (use_preferred) {
        preferred_operators = collect_preferred_operators(
            eval_context, preferred_operator_heuristics);
    }

    if (use_preferred && preferred_usage == PreferredUsage::PRUNE_BY_PREFERRED) {
        for (OperatorID op_id : preferred_operators) {
            insert_successor_into_open_list(
                eval_context, node_g, op_id, true);
        }
    } else {
        /* The successor ranking implied by RANK_BY_PREFERRED is done
           by the open list. */
        vector<OperatorID> successor_operators;
        g_successor_generator->generate_applicable_ops(
            eval_context.get_state(), successor_operators);
        for (OperatorID op_id : successor_operators) {
            bool preferred = use_preferred &&
                             preferred_operators.contains(op_id);
            insert_successor_into_open_list(
                eval_context, node_g, op_id, preferred);
        }
    }

    statistics.inc_expanded();
    node.close();
}

SearchStatus EnforcedHillClimbingSearch::step() {
    last_num_expanded = statistics.get_expanded();
    search_progress.check_progress(current_eval_context);

    if (check_goal_and_set_plan(current_eval_context.get_state())) {
        return SOLVED;
    }

    expand(current_eval_context);
    return ehc();
}

SearchStatus EnforcedHillClimbingSearch::ehc() {
    while (!open_list->empty()) {
        EdgeOpenListEntry entry = open_list->remove_min();
        StateID parent_state_id = entry.first;
        OperatorID last_op_id = entry.second;
        OperatorProxy last_op = task_proxy.get_operators()[last_op_id];

        GlobalState parent_state = state_registry.lookup_state(parent_state_id);
        SearchNode parent_node = search_space.get_node(parent_state);

        // d: distance from initial node in this EHC phase
        int d = parent_node.get_g() - current_phase_start_g +
                get_adjusted_cost(last_op);

        if (parent_node.get_real_g() + last_op.get_cost() >= bound)
            continue;

        GlobalState state = state_registry.get_successor_state(parent_state, last_op);
        statistics.inc_generated();

        SearchNode node = search_space.get_node(state);

        if (node.is_new()) {
            EvaluationContext eval_context(state, &statistics);
            reach_state(parent_state, last_op_id, state);
            statistics.inc_evaluated_states();

            if (eval_context.is_heuristic_infinite(heuristic)) {
                node.mark_as_dead_end();
                statistics.inc_dead_ends();
                continue;
            }

            int h = eval_context.get_heuristic_value(heuristic);
            node.open(parent_node, last_op);

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
                current_phase_start_g = node.get_g();
                return IN_PROGRESS;
            } else {
                expand(eval_context);
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

static shared_ptr<SearchEngine> _parse(OptionParser &parser) {
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
        return make_shared<EnforcedHillClimbingSearch>(opts);
}

static PluginShared<SearchEngine> _plugin("ehc", _parse);
}
