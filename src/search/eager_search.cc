#include "eager_search.h"

#include "evaluation_context.h"
#include "g_evaluator.h"
#include "globals.h"
#include "heuristic.h"
#include "option_parser.h"
#include "plugin.h"
#include "successor_generator.h"
#include "sum_evaluator.h"

#include <cassert>
#include <cstdlib>
#include <set>
using namespace std;

EagerSearch::EagerSearch(const Options &opts)
    : SearchEngine(opts),
      reopen_closed_nodes(opts.get<bool>("reopen_closed")),
      use_multi_path_dependence(opts.get<bool>("mpd")),
      open_list(opts.get<OpenList<StateID> *>("open")),
      f_evaluator(opts.get<ScalarEvaluator *>("f_eval", nullptr)),
      preferred_operator_heuristics(opts.get_list<Heuristic *>("preferred")) {
}

void EagerSearch::initialize() {
    cout << "Conducting best first search"
         << (reopen_closed_nodes ? " with" : " without")
         << " reopening closed nodes, (real) bound = " << bound
         << endl;
    if (use_multi_path_dependence)
        cout << "Using multi-path dependence (LM-A*)" << endl;
    assert(open_list);

    set<Heuristic *> hset;
    open_list->get_involved_heuristics(hset);

    // add heuristics that are used for preferred operators (in case they are
    // not also used in the open list)
    hset.insert(preferred_operator_heuristics.begin(),
                preferred_operator_heuristics.end());

    // add heuristics that are used in the f_evaluator. They are usually also
    // used in the open list and hence already be included, but we want to be
    // sure.
    if (f_evaluator) {
        f_evaluator->get_involved_heuristics(hset);
    }

    heuristics.assign(hset.begin(), hset.end());
    assert(!heuristics.empty());

    const GlobalState &initial_state = g_initial_state();
    // Note: we consider the initial state as reached by a preferred
    // operator.
    EvaluationContext eval_context(initial_state, 0, true, &statistics);

    statistics.inc_evaluated_states();

    if (open_list->is_dead_end(eval_context)) {
        cout << "Initial state is a dead end." << endl;
    } else {
        if (search_progress.check_progress(eval_context))
            print_checkpoint_line(0);
        start_f_value_statistics(eval_context);
        SearchNode node = search_space.get_node(initial_state);
        node.open_initial(eval_context.get_heuristic_value(heuristics[0]));

        open_list->insert(eval_context, initial_state.get_id());
    }

    print_initial_h_values(eval_context);
}

void EagerSearch::print_checkpoint_line(int g) const {
    cout << "[g=" << g << ", ";
    statistics.print_basic_statistics();
    cout << "]" << endl;
}

void EagerSearch::print_statistics() const {
    statistics.print_detailed_statistics();
    search_space.print_statistics();
}

SearchStatus EagerSearch::step() {
    pair<SearchNode, bool> n = fetch_next_node();
    if (!n.second) {
        return FAILED;
    }
    SearchNode node = n.first;

    GlobalState s = node.get_state();
    if (check_goal_and_set_plan(s))
        return SOLVED;

    vector<const GlobalOperator *> applicable_ops;
    set<const GlobalOperator *> preferred_ops;

    g_successor_generator->generate_applicable_ops(s, applicable_ops);
    // This evaluates the expanded state (again) to get preferred ops
    EvaluationContext eval_context(s, node.get_g(), false, &statistics);
    for (Heuristic *heur : preferred_operator_heuristics) {
        /* In an alternation search with unreliable heuristics, it is
           possible that this heuristic considers the state a dead
           end. We only want to ask for preferred operators for
           finite-value heuristics. */
        if (!eval_context.is_heuristic_infinite(heur)) {
            vector<const GlobalOperator *> preferred =
                eval_context.get_preferred_operators(heur);
            preferred_ops.insert(preferred.begin(), preferred.end());
        }
    }

    for (const GlobalOperator *op : applicable_ops) {
        if ((node.get_real_g() + op->get_cost()) >= bound)
            continue;

        GlobalState succ_state = g_state_registry->get_successor_state(s, *op);
        statistics.inc_generated();
        bool is_preferred = (preferred_ops.find(op) != preferred_ops.end());

        SearchNode succ_node = search_space.get_node(succ_state);

        // Previously encountered dead end. Don't re-evaluate.
        if (succ_node.is_dead_end())
            continue;

        // update new path
        if (use_multi_path_dependence || succ_node.is_new()) {
            /*
              Note: we must call reach_state for each heuristic, so
              don't break out of the for loop early.
            */
            bool h_is_dirty = false;
            for (Heuristic *heuristic : heuristics) {
                if (heuristic->reach_state(s, *op, succ_state))
                    h_is_dirty = true;
            }
            if (h_is_dirty && use_multi_path_dependence)
                succ_node.set_h_dirty();
        }

        if (succ_node.is_new()) {
            // We have not seen this state before.
            // Evaluate and create a new node.

            // Careful: succ_node.get_g() is not available here yet,
            // hence the stupid computation of succ_g.
            // TODO: Make this less fragile.
            int succ_g = node.get_g() + get_adjusted_cost(*op);

            EvaluationContext eval_context(
                succ_state, succ_g, is_preferred, &statistics);
            statistics.inc_evaluated_states();
            succ_node.clear_h_dirty();

            if (open_list->is_dead_end(eval_context)) {
                succ_node.mark_as_dead_end();
                statistics.inc_dead_ends();
                continue;
            }

            int succ_h = eval_context.get_heuristic_value(heuristics[0]);
            succ_node.open(succ_h, node, op);

            open_list->insert(eval_context, succ_state.get_id());
            if (search_progress.check_progress(eval_context)) {
                print_checkpoint_line(succ_node.get_g());
                reward_progress();
            }
        } else if (succ_node.get_g() > node.get_g() + get_adjusted_cost(*op)) {
            // We found a new cheapest path to an open or closed state.
            if (reopen_closed_nodes) {
                if (succ_node.is_closed()) {
                    /*
                      TODO: It would be nice if we had a way to test
                      that reopening is expected behaviour, i.e., exit
                      with an error when this is something where
                      reopening should not occur (e.g. A* with a
                      consistent heuristic).
                    */
                    statistics.inc_reopened();
                }
                succ_node.reopen(node, op);

                EvaluationContext eval_context(
                    succ_state, succ_node.get_g(), is_preferred, &statistics);

                /*
                  Note: our old code used to retrieve the h value from
                  the search node here. Our new code recomputes it as
                  necessary, thus avoiding the incredible ugliness of
                  the old "set_evaluator_value" approach, which also
                  did not generalize properly to settings with more
                  than one heuristic.

                  Reopening should not happen all that frequently, so
                  the performance impact of this is hopefully not that
                  large. In the medium term, we want the heuristics to
                  remember heuristic values for states themselves if
                  desired by the user, so that such recomputations
                  will just involve a look-up by the Heuristic object
                  rather than a recomputation of the heuristic value
                  from scratch.
                */
                open_list->insert(eval_context, succ_state.get_id());
            } else {
                // If we do not reopen closed nodes, we just update the parent pointers.
                // Note that this could cause an incompatibility between
                // the g-value and the actual path that is traced back.
                succ_node.update_parent(node, op);
            }
        }
    }

    return IN_PROGRESS;
}

pair<SearchNode, bool> EagerSearch::fetch_next_node() {
    /* TODO: The bulk of this code deals with multi-path dependence,
       which is a bit unfortunate since that is a special case that
       makes the common case look more complicated than it would need
       to be. We could refactor this by implementing multi-path
       dependence as a separate search algorithm that wraps the "usual"
       search algorithm and adds the extra processing in the desired
       places. I think this would lead to much cleaner code. */

    while (true) {
        if (open_list->empty()) {
            cout << "Completely explored state space -- no solution!" << endl;
            // HACK! HACK! we do this because SearchNode has no default/copy constructor
            SearchNode dummy_node = search_space.get_node(g_initial_state());
            return make_pair(dummy_node, false);
        }
        vector<int> last_key_removed;
        StateID id = open_list->remove_min(
            use_multi_path_dependence ? &last_key_removed : nullptr);
        // TODO is there a way we can avoid creating the state here and then
        //      recreate it outside of this function with node.get_state()?
        //      One way would be to store GlobalState objects inside SearchNodes
        //      instead of StateIDs
        GlobalState s = g_state_registry->lookup_state(id);
        SearchNode node = search_space.get_node(s);

        if (node.is_closed())
            continue;

        if (use_multi_path_dependence) {
            assert(last_key_removed.size() == 2);
            if (node.is_dead_end())
                continue;
            int pushed_h = last_key_removed[1];
            assert(node.get_h() >= pushed_h);
            if (node.get_h() > pushed_h) {
                // cout << "LM-A* skip h" << endl;
                continue;
            }
            assert(node.get_h() == pushed_h);
            if (!node.is_closed() && node.is_h_dirty()) {
                EvaluationContext eval_context(
                    node.get_state(), node.get_g(), false, &statistics);
                node.clear_h_dirty();

                if (open_list->is_dead_end(eval_context)) {
                    node.mark_as_dead_end();
                    statistics.inc_dead_ends();
                    continue;
                }

                int new_h = eval_context.get_heuristic_value(heuristics[0]);
                if (new_h > node.get_h()) {
                    assert(node.is_open());
                    node.increase_h(new_h);
                    open_list->insert(eval_context, node.get_state_id());
                    continue;
                }
            }
        }

        node.close();
        assert(!node.is_dead_end());
        update_f_value_statistics(node);
        statistics.inc_expanded();
        return make_pair(node, true);
    }
}

void EagerSearch::reward_progress() {
    // Boost the "preferred operator" open lists somewhat whenever
    // one of the heuristics finds a state with a new best h value.
    open_list->boost_preferred();
}

void EagerSearch::dump_search_space() const {
    search_space.dump();
}

void EagerSearch::start_f_value_statistics(EvaluationContext &eval_context) {
    if (f_evaluator) {
        int f_value = eval_context.get_heuristic_value(f_evaluator);
        statistics.report_f_value_progress(f_value);
    }
}

void EagerSearch::update_f_value_statistics(const SearchNode &node) {
    if (f_evaluator) {
        /*
          TODO: This code doesn't fit the idea of supporting
          an arbitrary f evaluator.
        */
        int new_f_value = node.get_g() + node.get_h();
        statistics.report_f_value_progress(new_f_value);
    }
}

static SearchEngine *_parse(OptionParser &parser) {
    //open lists are currently registered with the parser on demand,
    //because for templated classes the usual method of registering
    //does not work:
    Plugin<OpenList<StateID> >::register_open_lists();

    parser.document_synopsis("Eager best-first search", "");

    parser.add_option<OpenList<StateID> *>("open", "open list");
    parser.add_option<bool>("reopen_closed",
                            "reopen closed nodes", "false");
    parser.add_option<ScalarEvaluator *>(
        "f_eval",
        "set evaluator for jump statistics. "
        "(Optional; if no evaluator is used, jump statistics will not be displayed.)",
        OptionParser::NONE);
    parser.add_list_option<Heuristic *>(
        "preferred",
        "use preferred operators of these heuristics", "[]");
    SearchEngine::add_options_to_parser(parser);
    Options opts = parser.parse();

    EagerSearch *engine = nullptr;
    if (!parser.dry_run()) {
        opts.set<bool>("mpd", false);
        engine = new EagerSearch(opts);
    }

    return engine;
}

static SearchEngine *_parse_astar(OptionParser &parser) {
    parser.document_synopsis(
        "A* search (eager)",
        "A* is a special case of eager best first search that uses g+h "
        "as f-function. "
        "We break ties using the evaluator. Closed nodes are re-opened.");
    parser.document_note(
        "mpd option",
        "This option is currently only present for the A* algorithm and not "
        "for the more general eager search, "
        "because the current implementation of multi-path depedence "
        "does not support general open lists.");
    parser.document_note(
        "Equivalent statements using general eager search",
        "\n```\n--search astar(evaluator)\n```\n"
        "is equivalent to\n"
        "```\n--heuristic h=evaluator\n"
        "--search eager(tiebreaking([sum([g(), h]), h], unsafe_pruning=false),\n"
        "               reopen_closed=true, progress_evaluator=sum([g(), h]))\n"
        "```\n", true);
    parser.add_option<ScalarEvaluator *>("eval", "evaluator for h-value");
    parser.add_option<bool>("mpd",
                            "use multi-path dependence (LM-A*)", "false");
    SearchEngine::add_options_to_parser(parser);
    Options opts = parser.parse();

    EagerSearch *engine = nullptr;
    if (!parser.dry_run()) {
        GEvaluator *g = new GEvaluator();
        vector<ScalarEvaluator *> sum_evals;
        sum_evals.push_back(g);
        ScalarEvaluator *eval = opts.get<ScalarEvaluator *>("eval");
        sum_evals.push_back(eval);
        ScalarEvaluator *f_eval = new SumEvaluator(sum_evals);

        // use eval for tiebreaking
        std::vector<ScalarEvaluator *> evals;
        evals.push_back(f_eval);
        evals.push_back(eval);
        OpenList<StateID> *open = \
            new TieBreakingOpenList<StateID>(evals, false, false);

        opts.set("open", open);
        opts.set("f_eval", f_eval);
        opts.set("reopen_closed", true);
        vector<Heuristic *> preferred_list;
        opts.set("preferred", preferred_list);
        engine = new EagerSearch(opts);
    }

    return engine;
}

static SearchEngine *_parse_greedy(OptionParser &parser) {
    parser.document_synopsis("Greedy search (eager)", "");
    parser.document_note(
        "Open list",
        "In most cases, eager greedy best first search uses "
        "an alternation open list with one queue for each evaluator. "
        "If preferred operator heuristics are used, it adds an extra queue "
        "for each of these evaluators that includes only the nodes that "
        "are generated with a preferred operator. "
        "If only one evaluator and no preferred operator heuristic is used, "
        "the search does not use an alternation open list but a "
        "standard open list with only one queue.");
    parser.document_note(
        "Closed nodes",
        "Closed node are not re-opened");
    parser.document_note(
        "Equivalent statements using general eager search",
        "\n```\n--heuristic h2=eval2\n"
        "--search eager_greedy([eval1, h2], preferred=h2, boost=100)\n```\n"
        "is equivalent to\n"
        "```\n--heuristic h1=eval1 --heuristic h2=eval2\n"
        "--search eager(alt([single(h1), single(h1, pref_only=true), single(h2), \n"
        "                    single(h2, pref_only=true)], boost=100),\n"
        "               preferred=h2)\n```\n"
        "------------------------------------------------------------\n"
        "```\n--search eager_greedy([eval1, eval2])\n```\n"
        "is equivalent to\n"
        "```\n--search eager(alt([single(eval1), single(eval2)]))\n```\n"
        "------------------------------------------------------------\n"
        "```\n--heuristic h1=eval1\n"
        "--search eager_greedy(h1, preferred=h1)\n```\n"
        "is equivalent to\n"
        "```\n--heuristic h1=eval1\n"
        "--search eager(alt([single(h1), single(h1, pref_only=true)]),\n"
        "               preferred=h1)\n```\n"
        "------------------------------------------------------------\n"
        "```\n--search eager_greedy(eval1)\n```\n"
        "is equivalent to\n"
        "```\n--search eager(single(eval1))\n```\n", true);

    parser.add_list_option<ScalarEvaluator *>("evals", "scalar evaluators");
    parser.add_list_option<Heuristic *>(
        "preferred",
        "use preferred operators of these heuristics", "[]");
    parser.add_option<int>(
        "boost",
        "boost value for preferred operator open lists", "0");
    SearchEngine::add_options_to_parser(parser);


    Options opts = parser.parse();
    opts.verify_list_non_empty<ScalarEvaluator *>("evals");

    EagerSearch *engine = nullptr;
    if (!parser.dry_run()) {
        vector<ScalarEvaluator *> evals =
            opts.get_list<ScalarEvaluator *>("evals");
        vector<Heuristic *> preferred_list =
            opts.get_list<Heuristic *>("preferred");
        OpenList<StateID> *open;
        if ((evals.size() == 1) && preferred_list.empty()) {
            open = new StandardScalarOpenList<StateID>(evals[0], false);
        } else {
            vector<OpenList<StateID> *> inner_lists;
            for (ScalarEvaluator *evaluator : evals) {
                inner_lists.push_back(
                    new StandardScalarOpenList<StateID>(evaluator, false));
                if (!preferred_list.empty()) {
                    inner_lists.push_back(
                        new StandardScalarOpenList<StateID>(evaluator, true));
                }
            }
            open = new AlternationOpenList<StateID>(
                inner_lists, opts.get<int>("boost"));
        }

        opts.set("open", open);
        opts.set("reopen_closed", false);
        opts.set("mpd", false);
        ScalarEvaluator *evaluator = nullptr;
        opts.set("f_eval", evaluator);
        opts.set("preferred", preferred_list);
        engine = new EagerSearch(opts);
    }
    return engine;
}

static Plugin<SearchEngine> _plugin("eager", _parse);
static Plugin<SearchEngine> _plugin_astar("astar", _parse_astar);
static Plugin<SearchEngine> _plugin_greedy("eager_greedy", _parse_greedy);
