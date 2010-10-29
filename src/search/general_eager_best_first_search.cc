#include "general_eager_best_first_search.h"

#include "globals.h"
#include "heuristic.h"
#include "open_list_parser.h"
#include "option_parser.h"
#include "successor_generator.h"
#include "g_evaluator.h"
#include "sum_evaluator.h"
#include "open_lists/tiebreaking_open_list.h"

#include <cassert>
#include <cstdlib>
#include <set>
using namespace std;

GeneralEagerBestFirstSearch::GeneralEagerBestFirstSearch(
    OpenList<state_var_t *> *open,
    bool reopen_closed, bool pathmax_correction, ScalarEvaluator *f_eval, int g_bound)
    : reopen_closed_nodes(reopen_closed), do_pathmax(pathmax_correction),
      open_list(open), f_evaluator(f_eval) {
    bound = g_bound;
}

void
GeneralEagerBestFirstSearch::set_pref_operator_heuristics(
    vector<Heuristic *> &heur) {
    preferred_operator_heuristics = heur;
}

void GeneralEagerBestFirstSearch::initialize() {
    //TODO children classes should output which kind of search
    cout << "Conducting best first search"
         << (reopen_closed_nodes ? " with" : " without")
         << " reopening closed nodes, bound = " << bound
         << endl;
    if (do_pathmax)
        cout << "Using pathmax correction" << endl;
    assert(open_list != NULL);

    set<Heuristic *> hset;
    open_list->get_involved_heuristics(hset);

    for (set<Heuristic *>::iterator it = hset.begin(); it != hset.end(); it++) {
        estimate_heuristics.push_back(*it);
        search_progress.add_heuristic(*it);
    }

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

    for (set<Heuristic *>::iterator it = hset.begin(); it != hset.end(); it++) {
        heuristics.push_back(*it);
    }

    assert(heuristics.size() > 0);

    for (unsigned int i = 0; i < heuristics.size(); i++)
        heuristics[i]->evaluate(*g_initial_state);
    open_list->evaluate(0, false);
    search_progress.inc_evaluated();

    if (open_list->is_dead_end()) {
        assert(open_list->dead_end_is_reliable());
        cout << "Initial state is a dead end." << endl;
    } else {
        search_progress.get_initial_h_values();
        if (f_evaluator) {
            f_evaluator->evaluate(0, false);
            search_progress.report_f_value(f_evaluator->get_value());
        }
        search_progress.check_h_progress(0);
        SearchNode node = search_space.get_node(*g_initial_state);
        node.open_initial(heuristics[0]->get_value());

        open_list->insert(node.get_state_buffer());
    }
}


void GeneralEagerBestFirstSearch::statistics() const {
    search_progress.print_statistics();
    search_space.statistics();
}

int GeneralEagerBestFirstSearch::step() {
    pair<SearchNode, bool> n = fetch_next_node();
    if (!n.second) {
        return FAILED;
    }
    SearchNode node = n.first;

    State s = node.get_state();
    if (check_goal_and_set_plan(s))
        return SOLVED;

    vector<const Operator *> applicable_ops;
    set<const Operator *> preferred_ops;

    g_successor_generator->generate_applicable_ops(s, applicable_ops);
    // This evaluates the expanded state (again) to get preferred ops
    for (int i = 0; i < preferred_operator_heuristics.size(); i++) {
        vector<const Operator *> pref;
        pref.clear();
        preferred_operator_heuristics[i]->evaluate(s);
        preferred_operator_heuristics[i]->get_preferred_operators(pref);
        for (int i = 0; i < pref.size(); i++) {
            preferred_ops.insert(pref[i]);
        }
    }

    for (int i = 0; i < applicable_ops.size(); i++) {
        const Operator *op = applicable_ops[i];

        if ((node.get_g() + op->get_cost()) >= bound)
            continue;

        State succ_state(s, *op);
        search_progress.inc_generated();
        bool is_preferred = (preferred_ops.find(op) != preferred_ops.end());

        SearchNode succ_node = search_space.get_node(succ_state);

        if (succ_node.is_dead_end()) {
            // Previously encountered dead end. Don't re-evaluate.
            continue;
        } else if (succ_node.is_new()) {
            // We have not seen this state before.
            // Evaluate and create a new node.
            for (unsigned int i = 0; i < heuristics.size(); i++) {
                heuristics[i]->reach_state(s, *op, succ_node.get_state());
                heuristics[i]->evaluate(succ_state);
            }
            search_progress.inc_evaluated();

            // Note that we cannot use succ_node.get_g() here as the
            // node is not yet open. Furthermore, we cannot open it
            // before having checked that we're not in a dead end. The
            // division of responsibilities is a bit tricky here -- we
            // may want to refactor this later.
            open_list->evaluate(node.get_g() + op->get_cost(), is_preferred);
            bool dead_end = open_list->is_dead_end() && open_list->dead_end_is_reliable();
            if (dead_end) {
                succ_node.mark_as_dead_end();
                continue;
            }

            //TODO:CR - add an ID to each state, and then we can use a vector to save per-state information
            int succ_h = heuristics[0]->get_heuristic();
            if (do_pathmax) {
                if ((node.get_h() - op->get_cost()) > succ_h) {
                    //cout << "Pathmax correction: " << succ_h << " -> " << node.get_h() - op->get_cost() << endl;
                    succ_h = node.get_h() - op->get_cost();
                    heuristics[0]->set_evaluator_value(succ_h);
                    search_progress.inc_pathmax_corrections();
                }
            }
            succ_node.open(succ_h, node, op);

            open_list->insert(succ_node.get_state_buffer());
            search_progress.check_h_progress(succ_node.get_g());
        } else if (succ_node.get_g() > node.get_g() + op->get_cost()) {
            // We found a new cheapest path to an open or closed state.
            if (reopen_closed_nodes) {
                //TODO:CR - test if we should add a reevaluate flag and if it helps
                // if we reopen closed nodes, do that
                if (succ_node.is_closed()) {
                    /* TODO: Verify that the heuristic is inconsistent.
                     * Otherwise, this is a bug. This is a serious
                     * assertion because it can show that a heuristic that
                     * was thought to be consistent isn't. Therefore, it
                     * should be present also in release builds, so don't
                     * use a plain assert. */
                    //TODO:CR - add a consistent flag to heuristics, and add an assert here based on it
                    search_progress.inc_reopened();
                }
                succ_node.reopen(node, op);
                heuristics[0]->set_evaluator_value(succ_node.get_h());
                // TODO: this appears fishy to me. Why is here only heuristic[0]
                // involved? Is this still feasible in the current version?
                open_list->evaluate(succ_node.get_g(), is_preferred);

                open_list->insert(succ_node.get_state_buffer());
            } else {
                // if we do not reopen closed nodes, we just update the parent pointers
                // Note that this could cause an incompatibility between
                // the g-value and the actual path that is traced back
                succ_node.update_parent(node, op);
            }
        }
    }

    return IN_PROGRESS;
}

pair<SearchNode, bool> GeneralEagerBestFirstSearch::fetch_next_node() {
    while (true) {
        if (open_list->empty()) {
            cout << "Completely explored state space -- no solution!" << endl;
            return make_pair(search_space.get_node(*g_initial_state), false);
            //assert(false);
            //exit(1); // fix segfault in release mode
            // TODO: Deal with this properly. step() should return
            //       failure.
        }
        State state(open_list->remove_min());
        SearchNode node = search_space.get_node(state);

        // If the node is closed, we do not reopen it, as our heuristic
        // is consistent.
        // TODO: check this
        if (!node.is_closed()) {
            node.close();
            assert(!node.is_dead_end());
            update_jump_statistic(node);
            search_progress.inc_expanded();
            return make_pair(node, true);
        }
    }
}

void GeneralEagerBestFirstSearch::dump_search_space() {
    search_space.dump();
}

void GeneralEagerBestFirstSearch::update_jump_statistic(const SearchNode &node) {
    if (f_evaluator) {
        heuristics[0]->set_evaluator_value(node.get_h());
        f_evaluator->evaluate(node.get_g(), false);
        int new_f_value = f_evaluator->get_value();
        search_progress.report_f_value(new_f_value);
    }
}

void GeneralEagerBestFirstSearch::print_heuristic_values(const vector<int> &values) const {
    for (int i = 0; i < values.size(); i++) {
        cout << values[i];
        if (i != values.size() - 1)
            cout << "/";
    }
}

SearchEngine *GeneralEagerBestFirstSearch::create(const vector<string> &config,
                                                  int start, int &end,
                                                  bool dry_run) {
    if (config[start + 1] != "(")
        throw ParseError(start + 1);

    OpenListParser<state_var_t *> *p = OpenListParser<state_var_t *>::instance();
    OpenList<state_var_t *> *open = p->parse_open_list(config, start + 2, end,
                                                       dry_run);

    end++;
    if (end >= config.size())
        throw ParseError(end);

    // parse options
    bool reopen_closed = false;
    bool pathmax = false;
    ScalarEvaluator *f_eval = 0;
    vector<Heuristic *> preferred_list;
    int g_bound = numeric_limits<int>::max();

    if (config[end] != ")") {
        end++;
        NamedOptionParser option_parser;
        option_parser.add_bool_option("reopen_closed", &reopen_closed,
                                      "reopen closed nodes");
        option_parser.add_bool_option("pathmax", &pathmax,
                                      "use pathmax correction");
        option_parser.add_scalar_evaluator_option(
            "progress_evaluator", &f_eval, "set evaluator for jump statistics", true);
        option_parser.add_int_option("bound", &g_bound,
                                     "depth bound on g-values", true);
        option_parser.add_heuristic_list_option("preferred",
                                                &preferred_list, "use preferred operators of these heuristics");

        option_parser.parse_options(config, end, end, dry_run);
        end++;
    }
    if (config[end] != ")")
        throw ParseError(end);

    GeneralEagerBestFirstSearch *engine = 0;
    if (!dry_run) {
        engine = new GeneralEagerBestFirstSearch(open, reopen_closed,
                                                 pathmax, f_eval, g_bound);
        engine->set_pref_operator_heuristics(preferred_list);
    }

    return engine;
}

SearchEngine *GeneralEagerBestFirstSearch::create_astar(
    const vector<string> &config, int start, int &end, bool dry_run) {
    if (config[start + 1] != "(")
        throw ParseError(start + 1);

    ScalarEvaluator *eval = OptionParser::instance()->parse_scalar_evaluator(
        config, start + 2, end, dry_run);
    end++;

    bool pathmax = false;

    if (config[end] != ")") {
        end++;
        NamedOptionParser option_parser;

        option_parser.add_bool_option("pathmax", &pathmax,
                                      "use pathmax correction");

        option_parser.parse_options(config, end, end, dry_run);
        end++;
    }
    if (config[end] != ")")
        throw ParseError(end);


    GeneralEagerBestFirstSearch *engine = 0;
    if (!dry_run) {
        GEvaluator *g = new GEvaluator();
        vector<ScalarEvaluator *> sum_evals;
        sum_evals.push_back(g);
        sum_evals.push_back(eval);
        SumEvaluator *f_eval = new SumEvaluator(sum_evals);

        // use eval for tiebreaking
        std::vector<ScalarEvaluator *> evals;
        evals.push_back(f_eval);
        evals.push_back(eval);
        OpenList<state_var_t *> *open = \
            new TieBreakingOpenList<state_var_t *>(evals, false, false);

        engine = new GeneralEagerBestFirstSearch(open, true, pathmax,
                                                 f_eval, numeric_limits<int>::max());
    }

    return engine;
}

SearchEngine *GeneralEagerBestFirstSearch::create_greedy(
    const vector<string> &config, int start, int &end, bool dry_run) {
    if (config[start + 1] != "(")
        throw ParseError(start + 1);

    vector<ScalarEvaluator *> evals;
    OptionParser::instance()->parse_scalar_evaluator_list(config, start + 2,
                                                          end, false, evals,
                                                          dry_run);
    if (evals.empty())
        throw ParseError(end);
    end++;

    vector<Heuristic *> preferred_list;
    int boost = 1000;
    int g_bound = numeric_limits<int>::max();

    if (config[end] != ")") {
        end++;
        NamedOptionParser option_parser;
        option_parser.add_heuristic_list_option("preferred",
                                                &preferred_list, "use preferred operators of these heuristics");
        option_parser.add_int_option("boost", &boost,
                                     "boost value for successful sub-open-lists");
        //option_parser.add_int_option("bound", &g_bound,
        //                             "depth bound on g-values",true);
        option_parser.parse_options(config, end, end, dry_run);
        end++;
    }
    if (config[end] != ")")
        throw ParseError(end);

    GeneralEagerBestFirstSearch *engine = 0;
    if (!dry_run) {
        OpenList<state_var_t *> *open;
        if ((evals.size() == 1) && preferred_list.empty()) {
            open = new StandardScalarOpenList<state_var_t *>(evals[0], false);
        } else {
            vector<OpenList<state_var_t *> *> inner_lists;
            for (int i = 0; i < evals.size(); i++) {
                inner_lists.push_back(
                    new StandardScalarOpenList<state_var_t *>(evals[i], false));
                if (!preferred_list.empty()) {
                    inner_lists.push_back(
                        new StandardScalarOpenList<state_var_t *>(evals[i],
                                                                  true));
                }
            }
            open = new AlternationOpenList<state_var_t *>(inner_lists, boost);
        }

        engine = new GeneralEagerBestFirstSearch(open, false, false,
                                                 NULL, g_bound);
        engine->set_pref_operator_heuristics(preferred_list);
    }
    return engine;
}
