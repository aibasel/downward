#ifndef LOW_LEVEL_SEARCH_H
#define LOW_LEVEL_SEARCH_H

#include <vector>

#include "closed_list.h"
#include "open_list.h"
#include "search_engine.h"
#include "partial_relaxation.h"
#include "partially_relaxed_state.h"

class Heuristic;
class Operator;

typedef pair<const PartiallyRelaxedState *, const Operator *> LowLevelOpenListEntry;

struct LowLevelOpenListInfo {
    LowLevelOpenListInfo(Heuristic *heur, bool only_pref);
    Heuristic *heuristic;
    bool only_preferred_operators;
    OpenList<LowLevelOpenListEntry> open;
    int priority; // low value indicates high priority
};

class LowLevelSearchEngine {
    PartialRelaxation relaxation;

    std::vector<Heuristic *> heuristics;
    std::vector<Heuristic *> preferred_operator_heuristics;
    std::vector<LowLevelOpenListInfo> open_lists;
    ClosedList<PartiallyRelaxedState, const Operator *> closed_list;

    std::vector<int> best_heuristic_values;
    int generated_states;

    bool is_dead_end();
    bool check_goal();
    int get_plan_length(const PartiallyRelaxedState &state);
    bool check_progress();
    void reward_progress();
    void generate_successors(const PartiallyRelaxedState &state,
	const PartiallyRelaxedState *parent_ptr);
    bool fetch_next_state(
	const PartiallyRelaxedState* &next_predecessor,
	const Operator *& next_operator, PartiallyRelaxedState &next_state);
    LowLevelOpenListInfo *select_open_queue();

    std::vector<const Operator *> preferred_operators;
    void mark_preferred(const Operator *op);
public:
    LowLevelSearchEngine(const PartialRelaxation &relaxation);
    ~LowLevelSearchEngine();
    void add_heuristic(Heuristic *heuristic, bool use_estimates,
		       bool use_preferred_operators);
    int search(const State &state);
    void statistics() const;

    void get_preferred_operators(std::vector<const Operator *> &preferred);
};

#endif
