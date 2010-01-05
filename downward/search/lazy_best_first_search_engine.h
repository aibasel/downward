#ifndef LAZYBESTFIRSTSEARCHENGINE_H_
#define LAZYBESTFIRSTSEARCHENGINE_H_

#include "general_lazy_best_first_search.h"

class LazyBestFirstSearchEngine: public GeneralLazyBestFirstSearch {
protected:
    virtual void initialize();
public:
    LazyBestFirstSearchEngine();
    virtual ~LazyBestFirstSearchEngine();
};

#endif
