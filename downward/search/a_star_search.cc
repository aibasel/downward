#include "a_star_search.h"

#include "heuristic.h"
#include "g_evaluator.h"
#include "sum_evaluator.h"
#include "open_lists/tiebreaking_open_list.h"

AStarSearchEngine::AStarSearchEngine() :
    GeneralEagerBestFirstSearch(true) {
}

AStarSearchEngine::~AStarSearchEngine() {
    // TODO here, we could delete all the evaluators
    // that we created in initialize()
}

void AStarSearchEngine::initialize() {
    if (heuristics.size() > 1) {
        cerr << "Error: only one heuristic is allowed in A*" << endl;
        exit(1);
    }

    GEvaluator *g = new GEvaluator();
    SumEvaluator *f = new SumEvaluator();
    f->add_evaluator(g);
    f->add_evaluator(heuristics[0]);
    set_f_evaluator(f); // f = g + h

    // use h for tiebreaking
    std::vector<ScalarEvaluator *> evals;
    evals.push_back(f);
    evals.push_back(heuristics[0]);
    set_open_list(new TieBreakingOpenList<state_var_t *>(evals));

    GeneralEagerBestFirstSearch::initialize();
}
