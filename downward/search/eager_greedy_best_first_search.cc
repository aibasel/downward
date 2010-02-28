#include "eager_greedy_best_first_search.h"

#include "heuristic.h"
#include "open_lists/standard_scalar_open_list.h"
#include "open_lists/alternation_open_list.h"
#include "open_lists/tiebreaking_open_list.h"

EagerGreedyBestFirstSearchEngine::EagerGreedyBestFirstSearchEngine():
	GeneralEagerBestFirstSearch(false)  {
}

EagerGreedyBestFirstSearchEngine::~EagerGreedyBestFirstSearchEngine() {
}

void EagerGreedyBestFirstSearchEngine::initialize() {
	assert(heuristics.size() > 0);

	OpenList<state_var_t *> *ol;

    if (heuristics.size() + preferred_operator_heuristics.size() == 1) {
        ol = new StandardScalarOpenList<state_var_t *>(heuristics[0]);
    }
    else {
        vector<OpenList<state_var_t *>*> inner_lists;
        for (int i = 0; i < heuristics.size(); i++) {
            inner_lists.push_back(new StandardScalarOpenList<state_var_t *>(heuristics[i], false));
        }
        for (int i = 0; i < preferred_operator_heuristics.size(); i++) {
            inner_lists.push_back(new StandardScalarOpenList<state_var_t *>(preferred_operator_heuristics[i], true));
        }
        ol = new AlternationOpenList<state_var_t *>(inner_lists);
    }
    set_open_list(ol);

    GeneralEagerBestFirstSearch::initialize();
}
