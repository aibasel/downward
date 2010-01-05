#ifndef LAZYBESTFIRSTSEARCHENGINE_H
#define LAZYBESTFIRSTSEARCHENGINE_H

#include "general_lazy_best_first_search.h"

class LazyBestFirstSearchEngine: public GeneralLazyBestFirstSearch {
protected:
    virtual void initialize();
public:
    LazyBestFirstSearchEngine();
    virtual ~LazyBestFirstSearchEngine();
};

#endif
