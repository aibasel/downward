#ifndef EAGER_GREEDY_BEST_FIRST_SEARCH_H
#define EAGER_GREEDY_BEST_FIRST_SEARCH_H

#include "general_eager_best_first_search.h"

class Heuristic;
class Operator;

class EagerGreedyBestFirstSearchEngine : public GeneralEagerBestFirstSearch {
public:
    EagerGreedyBestFirstSearchEngine();
    ~EagerGreedyBestFirstSearchEngine();
    void initialize();
};

#endif
