#ifndef LAZY_WA_STAR_H
#define LAZY_WA_STAR_H

#include "general_lazy_best_first_search.h"

class LazyWeightedAStar: public GeneralLazyBestFirstSearch {
protected:
    int weight;
    virtual void initialize();
public:
    LazyWeightedAStar(int w);
    virtual ~LazyWeightedAStar();
};

#endif
