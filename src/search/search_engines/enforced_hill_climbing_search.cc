#include "enforced_hill_climbing_search.h"

#include "../option_parser.h"
#include "../plugin.h"

#include "../algorithms/ordered_set.h"
#include "../evaluators/g_evaluator.h"
#include "../evaluators/pref_evaluator.h"
#include "../open_lists/best_first_open_list.h"
#include "../open_lists/tiebreaking_open_list.h"
#include "../task_utils/successor_generator.h"
#include "../utils/logging.h"
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
    shared_ptr<Evaluator> g_evaluator = make_shared<GEval>();

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
        return make_shared<standard_scalar_open_list::BestFirstOpenListFactory>(options);
    } else {
        /*
          TODO: Reduce code duplication with search_common.cc,
          function create_astar_open_list_factory_and_f_eval.

          It would probably make sense to add a factory function or
          constructor that encapsulates this work to the tie-breaking
          open list code.
        */
        vector<shared_ptr<Evaluator>> evals = {g_evaluator, make_shared<PrefEval>()};
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
      evaluator(opts.get<shared_ptr<Evaluator>>("h")),
      preferred_operator_evaluators(opts.get_list<shared_ptr<Evaluator>>("preferred")),
      preferred_usage(opts.get<PreferredUsage>("preferred_usage")),
      current_eval_context(state_registry.get_initial_state(), &statistics),
      current_phase_start_g(-1),
      num_ehc_phases(0),
      last_num_expanded(-1) {
    for (const shared_ptr<Evaluator> &eval : preferred_operator_evaluators) {
        eval->get_path_dependent_evaluators(path_dependent_evaluators);
    }
    evaluator->get_path_dependent_evaluators(path_dependent_evaluators);

    const GlobalState &initial_state = state_registry.get_initial_state();
    for (Evaluator *evaluator : path_dependent_evaluators) {
        evaluator->notify_initial_state(initial_state);
    }
    use_preferred = find(preferred_operator_evaluators.begin(),
                         preferred_operator_evaluators.end(), evaluator) !=
        preferred_operator_evaluators.end();

    open_list = create_ehc_open_list_factory(
        use_preferred, preferred_usage)->create_edge_open_list();
}

EnforcedHillClimbingSearch::~EnforcedHillClimbingSearch() {
}

void EnforcedHillClimbingSearch::reach_state(
    const GlobalState &parent, OperatorID op_id, const GlobalState &state) {
    for (Evaluator *evaluator : path_dependent_evaluators) {
        evaluator->notify_state_transition(parent, op_id, state);
    }
}

void EnforcedHillClimbingSearch::initialize() {
    assert(evaluator);
    utils::g_log << "Conducting enforced hill-climbing search, (real) bound = "
                 << bound << endl;
    if (use_preferred) {
        utils::g_log << "Using preferred operators for "
                     << (preferred_usage == PreferredUsage::RANK_PREFERRED_FIRST ?
            "ranking successors" : "pruning") << endl;
    }

    bool dead_end = current_eval_context.is_evaluator_value_infinite(evaluator.get());
    statistics.inc_evaluated_states();
    print_initial_evaluator_values(current_eval_context);

    if (dead_end) {
        utils::g_log << "Initial state is a dead end, no solution" << endl;
        if (evaluator->dead_ends_are_reliable())
            utils::exit_with(ExitCode::SEARCH_UNSOLVABLE);
        else
            utils::exit_with(ExitCode::SEARCH_UNSOLVED_INCOMPLETE);
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
        for (const shared_ptr<Evaluator> &preferred_operator_evaluator : preferred_operator_evaluators) {
            collect_preferred_operators(eval_context,
                                        preferred_operator_evaluator.get(),
                                        preferred_operators);
        }
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
        successor_generator.generate_applicable_ops(
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

            if (eval_context.is_evaluator_value_infinite(evaluator.get())) {
                node.mark_as_dead_end();
                statistics.inc_dead_ends();
                continue;
            }

            int h = eval_context.get_evaluator_value(evaluator.get());
            node.open(parent_node, last_op, get_adjusted_cost(last_op));

            if (h < current_eval_context.get_evaluator_value(evaluator.get())) {
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
    utils::g_log << "No solution - FAILED" << endl;
    return FAILED;
}

void EnforcedHillClimbingSearch::print_statistics() const {
    statistics.print_detailed_statistics();

    utils::g_log << "EHC phases: " << num_ehc_phases << endl;
    assert(num_ehc_phases != 0);
    utils::g_log << "Average expansions per EHC phase: "
                 << static_cast<double>(statistics.get_expanded()) / num_ehc_phases
                 << endl;

    for (auto count : d_counts) {
        int depth = count.first;
        int phases = count.second.first;
        assert(phases != 0);
        int total_expansions = count.second.second;
        utils::g_log << "EHC phases of depth " << depth << ": " << phases
                     << " - Avg. Expansions: "
                     << static_cast<double>(total_expansions) / phases << endl;
    }
}

static shared_ptr<SearchEngine> _parse(OptionParser &parser) {
    parser.document_synopsis("Lazy enforced hill-climbing", "");
    parser.add_option<shared_ptr<Evaluator>>("h", "heuristic");
    vector<string> preferred_usages;
    preferred_usages.push_back("PRUNE_BY_PREFERRED");
    preferred_usages.push_back("RANK_PREFERRED_FIRST");
    parser.add_enum_option<PreferredUsage>(
        "preferred_usage",
        preferred_usages,
        "preferred operator usage",
        "PRUNE_BY_PREFERRED");
    parser.add_list_option<shared_ptr<Evaluator>>(
        "preferred",
        "use preferred operators of these evaluators",
        "[]");
    SearchEngine::add_options_to_parser(parser);
    Options opts = parser.parse();

    if (parser.dry_run())
        return nullptr;
    else
        return make_shared<EnforcedHillClimbingSearch>(opts);
}

static Plugin<SearchEngine> _plugin("ehc", _parse);
}
