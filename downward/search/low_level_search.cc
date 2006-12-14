#include "low_level_search.h"

#include "globals.h"
#include "heuristic.h"
#include "successor_generator.h"

#include <cassert>
using namespace std;

LowLevelOpenListInfo::LowLevelOpenListInfo(Heuristic *heur, bool only_pref) {
    heuristic = heur;
    only_preferred_operators = only_pref;
    priority = 0;
}

LowLevelSearchEngine::LowLevelSearchEngine(const PartialRelaxation &relax) 
    : relaxation(relax) {
    generated_states = 0;
}

LowLevelSearchEngine::~LowLevelSearchEngine() {
}

void LowLevelSearchEngine::add_heuristic(Heuristic *heuristic,
					  bool use_estimates,
					  bool use_preferred_operators) {
    assert(use_estimates || use_preferred_operators);
    heuristics.push_back(heuristic);
    best_heuristic_values.push_back(-1);
    if(use_estimates) {
	open_lists.push_back(LowLevelOpenListInfo(heuristic, false));
	open_lists.push_back(LowLevelOpenListInfo(heuristic, true));
    }
    if(use_preferred_operators)
	preferred_operator_heuristics.push_back(heuristic);
}

void LowLevelSearchEngine::statistics() const {
    cout << "Expanded " << closed_list.size() << " state(s)." << endl;
    cout << "Generated " << generated_states << " state(s)." << endl;
}

void LowLevelSearchEngine::mark_preferred(const Operator *op) {
    preferred_operators.push_back(op);
}

void LowLevelSearchEngine::get_preferred_operators(vector<const Operator *> &preferred) {
    preferred = preferred_operators;
}

int LowLevelSearchEngine::search(const State &state) {
    assert(!open_lists.empty());
    PartiallyRelaxedState current_state(relaxation, state);

    const PartiallyRelaxedState *current_predecessor = 0;
    const Operator *current_operator = 0;

    /* TODO: We are clearing all earlier open and closed queues, which
       is probably not a clever thing to do. In the future, might want
       to use some re-planning search algorithm that can deal with the
       previously collected information.
    */

    for(int i = 0; i < open_lists.size(); i++)
	open_lists[i].open.clear();
    closed_list.clear();

    preferred_operators.clear();

    for(;;) {
	if(!closed_list.contains(current_state)) {
	    // NOTE: Caching might be a bit more efficient if the
	    // cache info and closed list were merged into one data
	    // structure -- this would cut down on lookup overhead
	    // and on memory requirements.
	    const PartiallyRelaxedState *parent_ptr = closed_list.insert(
		current_state, current_predecessor, current_operator);
	    for(int i = 0; i < heuristics.size(); i++)
		heuristics[i]->evaluate(current_state);
	    if(!is_dead_end()) {
		if(check_goal())
		    return get_plan_length(current_state);
		if(check_progress())
		    reward_progress();
		generate_successors(current_state, parent_ptr);
	    }
	}
	if(!fetch_next_state(current_predecessor, current_operator, current_state))
	    return -1;
    }
}

bool LowLevelSearchEngine::is_dead_end() {    
    // If a reliable heuristic reports a dead end, we trust it.
    // Otherwise, all heuristics must agree on dead-end-ness.
    int dead_end_counter = 0;
    for(int i = 0; i < heuristics.size(); i++) {
	if(heuristics[i]->is_dead_end()) {
	    if(heuristics[i]->dead_ends_are_reliable())
		return true;
	    else
		dead_end_counter++;
	}
    }
    return dead_end_counter == heuristics.size();
}

bool LowLevelSearchEngine::check_goal() {
    // Any heuristic reports 0 iff this is a goal state, so we can
    // pick an arbitrary one.
    Heuristic *heur = open_lists[0].heuristic;
    if(heur->get_heuristic() == 0) {
	return true;
    } else {
	return false;
    }
}

int LowLevelSearchEngine::get_plan_length(const PartiallyRelaxedState &state) {
    // TODO:
    // Use the first action of the traced path as a preferred operator?
    // Or maybe even all applicable ones?
    vector<const Operator *> plan;
    closed_list.trace_path(state, plan);
    if(!plan.empty())
	mark_preferred(plan[0]);
    return plan.size();
}

bool LowLevelSearchEngine::check_progress() {
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

void LowLevelSearchEngine::reward_progress() {
    for(int i = 0; i < open_lists.size(); i++)
	if(open_lists[i].only_preferred_operators)
	    open_lists[i].priority -= 1000;
}  

void LowLevelSearchEngine::generate_successors(
    const PartiallyRelaxedState &state, const PartiallyRelaxedState *parent_ptr) {
    vector<const Operator *> all_operators;
    g_successor_generator->generate_applicable_ops(state, all_operators);

    vector<const Operator *> preferred_operators;
    for(int i = 0; i < preferred_operator_heuristics.size(); i++) {
	Heuristic *heur = preferred_operator_heuristics[i];
	if(!heur->is_dead_end())
	    heur->get_preferred_operators(preferred_operators);
    }

    for(int i = 0; i < open_lists.size(); i++) {
	Heuristic *heur = open_lists[i].heuristic;
	if(!heur->is_dead_end()) {
	    int h = heur->get_heuristic();
	    OpenList<LowLevelOpenListEntry> &open = open_lists[i].open;
	    vector<const Operator *> &ops =
		open_lists[i].only_preferred_operators ?
		preferred_operators : all_operators;
	    for(int j = 0; j < ops.size(); j++)
		open.insert(h, make_pair(parent_ptr, ops[j]));
	}
    }
    generated_states += all_operators.size();
}

bool LowLevelSearchEngine::fetch_next_state(
    const PartiallyRelaxedState* &next_predecessor,
    const Operator *& next_operator, PartiallyRelaxedState &next_state) {
    LowLevelOpenListInfo *open_info = select_open_queue();
    if(!open_info)
	return false;

    pair<const PartiallyRelaxedState *, const Operator *>
	next_pair = open_info->open.remove_min();
    open_info->priority++;

    next_predecessor = next_pair.first;
    next_operator = next_pair.second;
    next_state = PartiallyRelaxedState(*next_predecessor, *next_operator);
    return true;
}

LowLevelOpenListInfo *LowLevelSearchEngine::select_open_queue() {
    LowLevelOpenListInfo *best = 0;
    for(int i = 0; i < open_lists.size(); i++)
	if(!open_lists[i].open.empty() &&
	   (best == 0 || open_lists[i].priority < best->priority))
	    best = &open_lists[i];
    return best;
}
