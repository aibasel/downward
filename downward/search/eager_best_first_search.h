#ifndef EAGER_BEST_FIRST_SEARCH_H
#define EAGER_BEST_FIRST_SEARCH_H

#include "general_eager_best_first_search.h"

class Heuristic;
class Operator;

class EagerBestFirstSearchEngine : public GeneralEagerBestFirstSearch {
public:
    EagerBestFirstSearchEngine();
    ~EagerBestFirstSearchEngine();
    void initialize();
};

#endif
