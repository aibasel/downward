#ifndef A_STAR_SEARCH_H
#define A_STAR_SEARCH_H

#include "general_eager_best_first_search.h"

class Heuristic;
class Operator;

class AStarSearchEngine : public GeneralEagerBestFirstSearch {
public:
    AStarSearchEngine();
    ~AStarSearchEngine();
    virtual void initialize();
};

#endif
