#include "lazy_search.h"

#include "g_evaluator.h"
#include "globals.h"
#include "heuristic_cache.h"
#include "heuristic.h"
#include "plugin.h"
#include "rng.h"
#include "successor_generator.h"
#include "sum_evaluator.h"
#include "weighted_evaluator.h"

#include <algorithm>
#include <limits>

static const int DEFAULT_LAZY_BOOST = 1000;

LazySearch::LazySearch(const Options &opts)
    : SearchEngine(opts),
      open_list(opts.get<OpenList<OpenListEntryLazy> *>("open")),
      reopen_closed_nodes(opts.get<bool>("reopen_closed")),
      randomize_successors(opts.get<bool>("randomize_successors")),
      preferred_successors_first(opts.get<bool>("preferred_successors_first")),
      current_state(g_initial_state()),
      current_predecessor_id(StateID::no_state),
      current_operator(nullptr),
      current_g(0),
      current_real_g(0),
      current_eval_context(current_state, 0, true, &statistics) {
    /*
      We initialize current_eval_context in such a way that the initial node
      counts as "preferred".
    */
}

void LazySearch::set_pref_operator_heuristics(
    vector<Heuristic *> &heur) {
    preferred_operator_heuristics = heur;
}

void LazySearch::initialize() {
    cout << "Conducting lazy best first search, (real) bound = " << bound << endl;

    assert(open_list);
    set<Heuristic *> hset;
    open_list->get_involved_heuristics(hset);

    // Add heuristics that are used for preferred operators (in case they are
    // not also used in the open list).
    hset.insert(preferred_operator_heuristics.begin(),
                preferred_operator_heuristics.end());

    heuristics.assign(hset.begin(), hset.end());
    assert(!heuristics.empty());
}

void LazySearch::get_successor_operators(vector<const GlobalOperator *> &ops) {
    assert(ops.empty());

    vector<const GlobalOperator *> all_operators;
    g_successor_generator->generate_applicable_ops(
        current_state, all_operators);

    vector<const GlobalOperator *> preferred_operators;
    for (Heuristic *heur : preferred_operator_heuristics) {
        if (!current_eval_context.is_heuristic_infinite(heur)) {
            vector<const GlobalOperator *> preferred =
                current_eval_context.get_preferred_operators(heur);
            preferred_operators.insert(
                preferred_operators.end(), preferred.begin(), preferred.end());
        }
    }

    if (randomize_successors) {
        g_rng.shuffle(all_operators);
        // Note that preferred_operators can contain duplicates that are
        // only filtered out later, which gives operators "preferred
        // multiple times" a higher chance to be ordered early.
        g_rng.shuffle(preferred_operators);
    }

    if (preferred_successors_first) {
        for (const GlobalOperator *op : preferred_operators) {
            if (!op->is_marked()) {
                ops.push_back(op);
                op->mark();
            }
        }

        for (const GlobalOperator *op : all_operators)
            if (!op->is_marked())
                ops.push_back(op);
    } else {
        for (const GlobalOperator *op : preferred_operators)
            if (!op->is_marked())
                op->mark();
        ops.swap(all_operators);
    }
}

void LazySearch::generate_successors() {
    vector<const GlobalOperator *> operators;
    get_successor_operators(operators);
    statistics.inc_generated(operators.size());

    for (const GlobalOperator *op : operators) {
        int new_g = current_g + get_adjusted_cost(*op);
        int new_real_g = current_real_g + op->get_cost();
        bool is_preferred = op->is_marked();
        if (is_preferred)
            op->unmark();
        if (new_real_g < bound) {
            EvaluationContext new_eval_context(
                current_eval_context.get_cache(), new_g, is_preferred, nullptr);
            open_list->insert(new_eval_context, make_pair(current_state.get_id(), op));
        }
    }
}

SearchStatus LazySearch::fetch_next_state() {
    if (open_list->empty()) {
        cout << "Completely explored state space -- no solution!" << endl;
        return FAILED;
    }

    OpenListEntryLazy next = open_list->remove_min();

    current_predecessor_id = next.first;
    current_operator = next.second;
    GlobalState current_predecessor = g_state_registry->lookup_state(current_predecessor_id);
    assert(current_operator->is_applicable(current_predecessor));
    current_state = g_state_registry->get_successor_state(current_predecessor, *current_operator);

    SearchNode pred_node = search_space.get_node(current_predecessor);
    current_g = pred_node.get_g() + get_adjusted_cost(*current_operator);
    current_real_g = pred_node.get_real_g() + current_operator->get_cost();

    /*
      Note: We mark the node in current_eval_context as "preferred"
      here. This probably doesn't matter much either way because the
      node has already been selected for expansion, but eventually we
      should think more deeply about which path information to
      associate with the expanded vs. evaluated nodes in lazy search
      and where to obtain it from.
    */
    current_eval_context = EvaluationContext(current_state, current_g, true, &statistics);

    return IN_PROGRESS;
}

SearchStatus LazySearch::step() {
    // Invariants:
    // - current_state is the next state for which we want to compute the heuristic.
    // - current_predecessor is a permanent pointer to the predecessor of that state.
    // - current_operator is the operator which leads to current_state from predecessor.
    // - current_g is the g value of the current state according to the cost_type
    // - current_real_g is the g value of the current state (using real costs)


    SearchNode node = search_space.get_node(current_state);
    bool reopen = reopen_closed_nodes && !node.is_new() &&
                  !node.is_dead_end() && (current_g < node.get_g());

    if (node.is_new() || reopen) {
        StateID dummy_id = current_predecessor_id;
        // HACK! HACK! we do this because SearchNode has no default/copy constructor
        if (dummy_id == StateID::no_state) {
            dummy_id = g_initial_state().get_id();
        }
        GlobalState parent_state = g_state_registry->lookup_state(dummy_id);
        SearchNode parent_node = search_space.get_node(parent_state);

        if (current_operator) {
            for (Heuristic *heuristic : heuristics)
                heuristic->reach_state(parent_state, *current_operator, current_state);
        }
        statistics.inc_evaluated_states();
        if (!open_list->is_dead_end(current_eval_context)) {
            // TODO: Generalize code for using multiple heuristics.
            int h = current_eval_context.get_heuristic_value(heuristics[0]);
            if (reopen) {
                node.reopen(parent_node, current_operator);
                statistics.inc_reopened();
            } else if (current_predecessor_id == StateID::no_state) {
                node.open_initial(h);
                if (search_progress.check_progress(current_eval_context))
                    print_checkpoint_line(current_g);
            } else {
                node.open(h, parent_node, current_operator);
            }
            node.close();
            if (check_goal_and_set_plan(current_state))
                return SOLVED;
            if (search_progress.check_progress(current_eval_context)) {
                print_checkpoint_line(current_g);
                reward_progress();
            }
            generate_successors();
            statistics.inc_expanded();
        } else {
            node.mark_as_dead_end();
            statistics.inc_dead_ends();
        }
        if (current_predecessor_id == StateID::no_state) {
            print_initial_h_values(current_eval_context);
        }
    }
    return fetch_next_state();
}

void LazySearch::reward_progress() {
    open_list->boost_preferred();
}

void LazySearch::print_checkpoint_line(int g) const {
    cout << "[g=" << g << ", ";
    statistics.print_basic_statistics();
    cout << "]" << endl;
}

void LazySearch::print_statistics() const {
    statistics.print_detailed_statistics();
    search_space.print_statistics();
}


static void _add_succ_order_options(OptionParser &parser) {
    vector<string> options;
    parser.add_option<bool>(
        "randomize_successors",
        "randomize the order in which successors are generated",
        "false");
    parser.add_option<bool>(
        "preferred_successors_first",
        "consider preferred operators first",
        "false");
    parser.document_note(
        "Successor ordering",
        "When using randomize_successors=true and "
        "preferred_successors_first=true, randomization happens before "
        "preferred operators are moved to the front.");
}

static SearchEngine *_parse(OptionParser &parser) {
    parser.document_synopsis("Lazy best-first search", "");
    Plugin<OpenList<OpenListEntryLazy > >::register_open_lists();
    parser.add_option<OpenList<OpenListEntryLazy> *>("open", "open list");
    parser.add_option<bool>("reopen_closed", "reopen closed nodes", "false");
    parser.add_list_option<Heuristic *>(
        "preferred",
        "use preferred operators of these heuristics", "[]");
    _add_succ_order_options(parser);
    SearchEngine::add_options_to_parser(parser);
    Options opts = parser.parse();

    LazySearch *engine = nullptr;
    if (!parser.dry_run()) {
        engine = new LazySearch(opts);
        vector<Heuristic *> preferred_list =
            opts.get_list<Heuristic *>("preferred");
        engine->set_pref_operator_heuristics(preferred_list);
    }

    return engine;
}


static SearchEngine *_parse_greedy(OptionParser &parser) {
    parser.document_synopsis("Greedy search (lazy)", "");
    parser.document_note(
        "Open lists",
        "In most cases, lazy greedy best first search uses "
        "an alternation open list with one queue for each evaluator. "
        "If preferred operator heuristics are used, it adds an "
        "extra queue for each of these evaluators that includes "
        "only the nodes that are generated with a preferred operator. "
        "If only one evaluator and no preferred operator heuristic is used, "
        "the search does not use an alternation open list "
        "but a standard open list with only one queue.");
    parser.document_note(
        "Equivalent statements using general lazy search",
        "\n```\n--heuristic h2=eval2\n"
        "--search lazy_greedy([eval1, h2], preferred=h2, boost=100)\n```\n"
        "is equivalent to\n"
        "```\n--heuristic h1=eval1 --heuristic h2=eval2\n"
        "--search lazy(alt([single(h1), single(h1, pref_only=true), single(h2),\n"
        "                  single(h2, pref_only=true)], boost=100),\n"
        "              preferred=h2)\n```\n"
        "------------------------------------------------------------\n"
        "```\n--search lazy_greedy([eval1, eval2], boost=100)\n```\n"
        "is equivalent to\n"
        "```\n--search lazy(alt([single(eval1), single(eval2)], boost=100))\n```\n"
        "------------------------------------------------------------\n"
        "```\n--heuristic h1=eval1\n--search lazy_greedy(h1, preferred=h1)\n```\n"
        "is equivalent to\n"
        "```\n--heuristic h1=eval1\n"
        "--search lazy(alt([single(h1), single(h1, pref_only=true)], boost=1000),\n"
        "              preferred=h1)\n```\n"
        "------------------------------------------------------------\n"
        "```\n--search lazy_greedy(eval1)\n```\n"
        "is equivalent to\n"
        "```\n--search lazy(single(eval1))\n```\n",
        true);

    parser.add_list_option<ScalarEvaluator *>("evals", "scalar evaluators");
    parser.add_list_option<Heuristic *>(
        "preferred",
        "use preferred operators of these heuristics", "[]");
    parser.add_option<bool>("reopen_closed",
                            "reopen closed nodes", "false");
    parser.add_option<int>(
        "boost",
        "boost value for alternation queues that are restricted "
        "to preferred operator nodes",
        OptionParser::to_str(DEFAULT_LAZY_BOOST));
    _add_succ_order_options(parser);
    SearchEngine::add_options_to_parser(parser);
    Options opts = parser.parse();

    LazySearch *engine = 0;
    if (!parser.dry_run()) {
        vector<ScalarEvaluator *> evals =
            opts.get_list<ScalarEvaluator *>("evals");
        vector<Heuristic *> preferred_list =
            opts.get_list<Heuristic *>("preferred");
        OpenList<OpenListEntryLazy> *open;
        if ((evals.size() == 1) && preferred_list.empty()) {
            open = new StandardScalarOpenList<OpenListEntryLazy>(evals[0],
                                                                 false);
        } else {
            vector<OpenList<OpenListEntryLazy> *> inner_lists;
            for (ScalarEvaluator *eval : evals) {
                inner_lists.push_back(
                    new StandardScalarOpenList<OpenListEntryLazy>(eval, false));
                if (!preferred_list.empty()) {
                    inner_lists.push_back(
                        new StandardScalarOpenList<OpenListEntryLazy>(eval, true));
                }
            }
            open = new AlternationOpenList<OpenListEntryLazy>(
                inner_lists, opts.get<int>("boost"));
        }
        opts.set("open", open);
        engine = new LazySearch(opts);
        engine->set_pref_operator_heuristics(preferred_list);
    }
    return engine;
}

static SearchEngine *_parse_weighted_astar(OptionParser &parser) {
    parser.document_synopsis(
        "(Weighted) A* search (lazy)",
        "Weighted A* is a special case of lazy best first search.");
    parser.document_note(
        "Open lists",
        "In the general case, it uses an alternation open list "
        "with one queue for each evaluator h that ranks the nodes "
        "by g + w * h. If preferred operator heuristics are used, "
        "it adds for each of the evaluators another such queue that "
        "only inserts nodes that are generated by preferred operators. "
        "In the special case with only one evaluator and no preferred "
        "operator heuristics, it uses a single queue that "
        "is ranked by g + w * h. ");
    parser.document_note(
        "Equivalent statements using general lazy search",
        "\n```\n--heuristic h1=eval1\n"
        "--search lazy_wastar([h1, eval2], w=2, preferred=h1,\n"
        "                     bound=100, boost=500)\n```\n"
        "is equivalent to\n"
        "```\n--heuristic h1=eval1 --heuristic h2=eval2\n"
        "--search lazy(alt([single(sum([g(), weight(h1, 2)])),\n"
        "                   single(sum([g(), weight(h1, 2)]), pref_only=true),\n"
        "                   single(sum([g(), weight(h2, 2)])),\n"
        "                   single(sum([g(), weight(h2, 2)]), pref_only=true)],\n"
        "                  boost=500),\n"
        "              preferred=h1, reopen_closed=true, bound=100)\n```\n"
        "------------------------------------------------------------\n"
        "```\n--search lazy_wastar([eval1, eval2], w=2, bound=100)\n```\n"
        "is equivalent to\n"
        "```\n--search lazy(alt([single(sum([g(), weight(eval1, 2)])),\n"
        "                   single(sum([g(), weight(eval2, 2)]))],\n"
        "                  boost=1000),\n"
        "              reopen_closed=true, bound=100)\n```\n"
        "------------------------------------------------------------\n"
        "```\n--search lazy_wastar([eval1, eval2], bound=100, boost=0)\n```\n"
        "is equivalent to\n"
        "```\n--search lazy(alt([single(sum([g(), eval1])),\n"
        "                   single(sum([g(), eval2]))])\n"
        "              reopen_closed=true, bound=100)\n```\n"
        "------------------------------------------------------------\n"
        "```\n--search lazy_wastar(eval1, w=2)\n```\n"
        "is equivalent to\n"
        "```\n--search lazy(single(sum([g(), weight(eval1, 2)])), reopen_closed=true)\n```\n",
        true);

    parser.add_list_option<ScalarEvaluator *>("evals", "scalar evaluators");
    parser.add_list_option<Heuristic *>(
        "preferred",
        "use preferred operators of these heuristics", "[]");
    parser.add_option<bool>("reopen_closed", "reopen closed nodes", "true");
    parser.add_option<int>("boost",
                           "boost value for preferred operator open lists",
                           OptionParser::to_str(DEFAULT_LAZY_BOOST));
    parser.add_option<int>("w", "heuristic weight", "1");
    _add_succ_order_options(parser);
    SearchEngine::add_options_to_parser(parser);
    Options opts = parser.parse();

    opts.verify_list_non_empty<ScalarEvaluator *>("evals");

    LazySearch *engine = 0;
    if (!parser.dry_run()) {
        vector<ScalarEvaluator *> evals = opts.get_list<ScalarEvaluator *>("evals");
        vector<Heuristic *> preferred_list =
            opts.get_list<Heuristic *>("preferred");
        vector<OpenList<OpenListEntryLazy> *> inner_lists;
        for (size_t i = 0; i < evals.size(); ++i) {
            GEvaluator *g = new GEvaluator();
            vector<ScalarEvaluator *> sum_evals;
            sum_evals.push_back(g);
            if (opts.get<int>("w") == 1) {
                sum_evals.push_back(evals[i]);
            } else {
                WeightedEvaluator *w = new WeightedEvaluator(
                    evals[i],
                    opts.get<int>("w"));
                sum_evals.push_back(w);
            }
            SumEvaluator *f_eval = new SumEvaluator(sum_evals);

            inner_lists.push_back(
                new StandardScalarOpenList<OpenListEntryLazy>(f_eval, false));

            if (!preferred_list.empty()) {
                inner_lists.push_back(
                    new StandardScalarOpenList<OpenListEntryLazy>(f_eval,
                                                                  true));
            }
        }
        OpenList<OpenListEntryLazy> *open;
        if (inner_lists.size() == 1) {
            open = inner_lists[0];
        } else {
            open = new AlternationOpenList<OpenListEntryLazy>(
                inner_lists, opts.get<int>("boost"));
        }

        opts.set("open", open);

        engine = new LazySearch(opts);
        engine->set_pref_operator_heuristics(preferred_list);
    }
    return engine;
}

static Plugin<SearchEngine> _plugin("lazy", _parse);
static Plugin<SearchEngine> _plugin_greedy("lazy_greedy", _parse_greedy);
static Plugin<SearchEngine> _plugin_weighted_astar("lazy_wastar", _parse_weighted_astar);
