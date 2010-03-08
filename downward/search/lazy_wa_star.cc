#include "lazy_wa_star.h"
#include "open_lists/standard_scalar_open_list.h"
#include "open_lists/alternation_open_list.h"
#include "open_lists/tiebreaking_open_list.h"
#include "scalar_evaluator.h"
#include "heuristic.h"
#include "g_evaluator.h"
#include "sum_evaluator.h"
#include "weighted_evaluator.h"
#include <vector>

LazyWeightedAStar::LazyWeightedAStar(int w):
    GeneralLazyBestFirstSearch(true), weight(w) {
    // TODO Auto-generated constructor stub

}

LazyWeightedAStar::~LazyWeightedAStar() {
    // TODO Auto-generated destructor stub
}


void LazyWeightedAStar::initialize() {
    //TODO children classes should output which kind of search
    cout << "Conducting lazy weighted A* search, weight = " << weight <<  endl;

    GEvaluator *g = new GEvaluator();

    if ((heuristics.size() == 1) &&  (preferred_operator_heuristics.size() == 0)) {
        SumEvaluator *f = new SumEvaluator();
        WeightedEvaluator *w = new WeightedEvaluator(heuristics[0], weight);
        f->add_evaluator(g);
        f->add_evaluator(w);
        open_list = new StandardScalarOpenList<OpenListEntryLazy>(f);
    }
    else {
        vector<OpenList<OpenListEntryLazy>*> inner_lists;
        for (int i = 0; i < estimate_heuristics.size(); i++) {
            SumEvaluator *f = new SumEvaluator();
            WeightedEvaluator *w = new WeightedEvaluator(estimate_heuristics[i], weight);
            f->add_evaluator(g);
            f->add_evaluator(w);
            inner_lists.push_back(new StandardScalarOpenList<OpenListEntryLazy>(f, false));
            if (preferred_operator_heuristics.size() > 0) {
                inner_lists.push_back(new StandardScalarOpenList<OpenListEntryLazy>(f, true));
            }
        }
        open_list = new AlternationOpenList<OpenListEntryLazy>(inner_lists);
    }

    GeneralLazyBestFirstSearch::initialize();
}
