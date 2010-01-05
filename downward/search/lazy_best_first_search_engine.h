#ifndef LAZY_BEST_FIRST_SEARCH_ENGINE_H
#define LAZY_BEST_FIRST_SEARCH_ENGINE_H

#include "general_lazy_best_first_search.h"

class LazyBestFirstSearchEngine: public GeneralLazyBestFirstSearch {
protected:
    virtual void initialize();
public:
    LazyBestFirstSearchEngine();
    virtual ~LazyBestFirstSearchEngine();
};

#endif
