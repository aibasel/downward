#include "general_lazy_best_first_search.h"
#include "heuristic.h"
#include "successor_generator.h"
#include "open-lists/standard_scalar_open_list.h"
#include "open-lists/alternation_open_list.h"
#include "open-lists/tiebreaking_open_list.h"
#include "g_evaluator.h"
#include "sum_evaluator.h"

GeneralLazyBestFirstSearch::GeneralLazyBestFirstSearch():
    current_state(*g_initial_state) {
    generated_states = 0;
    current_predecessor = 0;
    current_operator = 0;
    current_g = 0;
}

GeneralLazyBestFirstSearch::~GeneralLazyBestFirstSearch() {
}

void GeneralLazyBestFirstSearch::set_open_list(OpenList<OpenListEntryWithG> *open) {
    open_list = open;
}

void GeneralLazyBestFirstSearch::initialize() {
    //TODO children classes should output which kind of search
    cout << "Conducting lazy best first search" << endl;

    //assert(open_list != NULL);
    assert(heuristics.size() > 0);

    //GEvaluator *g = new GEvaluator();

    if (heuristics.size() + preferred_operator_heuristics.size() == 1) {
        //SumEvaluator *f = new SumEvaluator();
        //f->add_evaluator(g);
        //f->add_evaluator(heuristics[0]);
        //open_list = new StandardScalarOpenList<OpenListEntryWithG>(f);

        open_list = new StandardScalarOpenList<OpenListEntryWithG>(heuristics[0]);
    }
    else {
        vector<OpenList<OpenListEntryWithG>*> inner_lists;
        for (int i = 0; i < heuristics.size(); i++) {
            //SumEvaluator *f = new SumEvaluator();
            //f->add_evaluator(g);
            //f->add_evaluator(heuristics[i]);
            //inner_lists.push_back(new StandardScalarOpenList<OpenListEntryWithG>(f, false));

            inner_lists.push_back(new StandardScalarOpenList<OpenListEntryWithG>(heuristics[i], false));
        }
        for (int i = 0; i < preferred_operator_heuristics.size(); i++) {
            //SumEvaluator *f = new SumEvaluator();
            //f->add_evaluator(g);
            //f->add_evaluator(preferred_operator_heuristics[i]);
            //inner_lists.push_back(new StandardScalarOpenList<OpenListEntryWithG>(f, true));

            inner_lists.push_back(new StandardScalarOpenList<OpenListEntryWithG>(preferred_operator_heuristics[i], true));
        }
        open_list = new AlternationOpenList<OpenListEntryWithG>(inner_lists);
    }
}

void GeneralLazyBestFirstSearch::add_heuristic(Heuristic *heuristic,
                      bool use_estimates,
                      bool use_preferred_operators) {
    assert(use_estimates || use_preferred_operators);
    if (use_estimates || use_preferred_operators) {
        heuristics.push_back(heuristic);
        best_heuristic_values.push_back(-1);
    }
    if(use_preferred_operators) {
        preferred_operator_heuristics.push_back(heuristic);
    }
}

bool GeneralLazyBestFirstSearch::check_goal() {
    // Any heuristic reports 0 iff this is a goal state, so we can
    // pick an arbitrary one. (only if there are no action costs)
    Heuristic *heur = heuristics[0];
    if(!heur->is_dead_end() && heur->get_heuristic() == 0) {
        // We actually need this silly !heur->is_dead_end() check because
        // this state *might* be considered a non-dead end by the
        // overall search even though heur considers it a dead end
        // (e.g. if heur is the CG heuristic, but the FF heuristic is
        // also computed and doesn't consider this state a dead end.
        // If heur considers the state a dead end, it cannot be a goal
        // state (heur will not be *that* stupid). We may not call
        // get_heuristic() in such cases because it will barf.
        if(g_use_metric) {
            bool is_goal = test_goal(current_state);
            if (!is_goal) return false;
        }

        cout << "Solution found! " << endl;
        Plan plan;
        closed_list.trace_path(current_state, plan);
        set_plan(plan);
        return true;
    } else {
        return false;
    }
}

void GeneralLazyBestFirstSearch::generate_successors(const State *parent_ptr) {
    vector<const Operator *> all_operators;
    vector<const Operator *> preferred_operators;

    g_successor_generator->generate_applicable_ops(current_state, all_operators);

    for(int i = 0; i < preferred_operator_heuristics.size(); i++) {
        Heuristic *heur = preferred_operator_heuristics[i];
        if(!heur->is_dead_end()) {
            heur->get_preferred_operators(preferred_operators);
        }
    }

    for(int i = 0; i < preferred_operators.size(); i++) {
        preferred_operators[i]->mark();
    }

    if(!open_list->is_dead_end()) {
        for(int j = 0; j < all_operators.size(); j++) {
            open_list->evaluate(0, all_operators[j]->is_marked()); // TODO: handle g in open list
            open_list->insert(
                    make_pair(make_pair(parent_ptr, all_operators[j]),
                            current_g + all_operators[j]->get_cost()));
        }
    }

    for(int i = 0; i < preferred_operators.size(); i++) {
        preferred_operators[i]->unmark();
    }
    generated_states += all_operators.size();
}

int GeneralLazyBestFirstSearch::fetch_next_state() {
    if(open_list->empty()) {
        cout << "Completely explored state space -- no solution!" << endl;
        return FAILED;
    }

    OpenListEntryWithG next = open_list->remove_min();

    current_predecessor = next.first.first;
    current_operator = next.first.second;
    current_g = next.second;
    current_state = State(*current_predecessor, *current_operator);


    return IN_PROGRESS;
}

int GeneralLazyBestFirstSearch::step() {
    // Invariants:
    // - current_state is the next state for which we want to compute the heuristic.
    // - current_predecessor is a permanent pointer to the predecessor of that state.
    // - current_operator is the operator which leads to current_state from predecessor.
    // - current_g is the g value of the current state

    if(!closed_list.contains(current_state)) {
        const State *parent_ptr = closed_list.insert(
        current_state, current_predecessor, current_operator);

        for(int i = 0; i < heuristics.size(); i++) {
            if (current_operator != NULL) {
                heuristics[i]->reach_state(*current_predecessor, *current_operator, *parent_ptr);
            }
            heuristics[i]->evaluate(current_state);
        }
        open_list->evaluate(current_g, false);
        if(!open_list->is_dead_end()) {
            if(check_goal())
                return SOLVED;
            if(check_progress()) {
                report_progress();
                reward_progress();
            }
            generate_successors(parent_ptr);
        }
    }
    return fetch_next_state();
}


bool GeneralLazyBestFirstSearch::check_progress() {
    bool progress = false;
    for(int i = 0; i < heuristics.size(); i++) {
    if(heuristics[i]->is_dead_end())
        continue;
    int h = heuristics[i]->get_heuristic();
    int &best_h = best_heuristic_values[i];
    if(best_h == -1 || h < best_h) {
        best_h = h;
        progress = true;
    }
    }
    return progress;
}

void GeneralLazyBestFirstSearch::report_progress() {
    cout << "Best heuristic value: ";
    for(int i = 0; i < heuristics.size(); i++) {
    cout << best_heuristic_values[i];
    if(i != heuristics.size() - 1)
        cout << "/";
    }
    cout << " [expanded " << closed_list.size() << " state(s)]" << endl;
}

void GeneralLazyBestFirstSearch::reward_progress() {
    // Boost the "preferred operator" open lists somewhat whenever
    // progress is made. This used to be used in multi-heuristic mode
    // only, but it is also useful in single-heuristic mode, at least
    // in Schedule.
    //
    // TODO: Test the impact of this, and find a better way of rewarding
    // successful exploration. For example, reward only the open queue
    // from which the good state was extracted and/or the open queues
    // for the heuristic for which a new best value was found.

    open_list->boost_preferred();
    //for(int i = 0; i < open_lists.size(); i++)
    //if(open_lists[i].only_preferred_operators)
    //    open_lists[i].priority -= 1000;
}

void GeneralLazyBestFirstSearch::statistics() const {
    cout << "Expanded " << closed_list.size() << " state(s)." << endl;
    cout << "Generated " << generated_states << " state(s)." << endl;
}
