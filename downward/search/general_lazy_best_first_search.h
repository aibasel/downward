#ifndef GENERALLAZYBESTFIRSTSEARCH_H_
#define GENERALLAZYBESTFIRSTSEARCH_H_

#include <vector>

#include "closed_list.h"
#include "open-lists/open_list.h"
#include "search_engine.h"
#include "state.h"
#include "scalar_evaluator.h"

class Heuristic;
class Operator;


typedef pair<const State *, const Operator *> OpenListEntry;
typedef pair<pair<const State *, const Operator *>, int> OpenListEntryWithG;

class GeneralLazyBestFirstSearch: public SearchEngine {
protected:
    ClosedList<State, const Operator *> closed_list;
    OpenList<OpenListEntryWithG> *open_list;
    ScalarEvaluator *f_evaluator;
    vector<Heuristic *> heuristics;
    vector<Heuristic *> preferred_operator_heuristics;

    vector<int> best_heuristic_values;
    int generated_states;

    State current_state;
    const State *current_predecessor;
    const Operator *current_operator;
    int current_g;

    virtual void initialize();
    virtual int step();

    void generate_successors(const State *parent_ptr);
    int fetch_next_state();

    bool check_goal();
    bool check_progress();
    void report_progress();
    void reward_progress();

    void set_open_list(OpenList<OpenListEntryWithG> *open);
public:
    GeneralLazyBestFirstSearch();
    virtual ~GeneralLazyBestFirstSearch();

    virtual void add_heuristic(Heuristic *heuristic, bool use_estimates,
                                   bool use_preferred_operators);

    virtual void statistics() const;
};

#endif /* GENERALLAZYBESTFIRSTSEARCH_H_ */
