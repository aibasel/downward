#include "eager_greedy_best_first_search.h"

#include "heuristic.h"
#include "open-lists/standard_scalar_open_list.h"
#include "open-lists/alternation_open_list.h"

EagerGreedyBestFirstSearchEngine::EagerGreedyBestFirstSearchEngine():
	GeneralEagerBestFirstSearch(false)  {
}

EagerGreedyBestFirstSearchEngine::~EagerGreedyBestFirstSearchEngine() {
}

void EagerGreedyBestFirstSearchEngine::initialize() {
	assert(heuristics.size() > 0);

	vector<OpenList<state_var_t *> *> open_lists;
	for (unsigned int i = 0; i < heuristics.size(); i++) {
		open_lists.push_back(new StandardScalarOpenList<state_var_t *>(heuristics[i]));
	}
    set_open_list(new AlternationOpenList<state_var_t *>(open_lists));

    GeneralEagerBestFirstSearch::initialize();
}
