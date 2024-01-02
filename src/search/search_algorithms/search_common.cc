#include "search_common.h"

#include "../open_list_factory.h"

#include "../evaluators/g_evaluator.h"
#include "../evaluators/sum_evaluator.h"
#include "../evaluators/weighted_evaluator.h"
#include "../plugins/options.h"
#include "../open_lists/alternation_open_list.h"
#include "../open_lists/best_first_open_list.h"
#include "../open_lists/tiebreaking_open_list.h"

#include <memory>

using namespace std;

namespace search_common {
static shared_ptr<TaskIndependentOpenListFactory> create_task_independent_alternation_open_list_factory_aux(
    const vector<shared_ptr<TaskIndependentEvaluator>> &evals,
    const vector<shared_ptr<TaskIndependentEvaluator>> &preferred_evaluators,
    int boost) {
    if (evals.size() == 1 && preferred_evaluators.empty()) {
        return make_shared<standard_scalar_open_list::TaskIndependentBestFirstOpenListFactory>(evals[0], false);
    } else {
        vector<shared_ptr<TaskIndependentOpenListFactory>> subfactories;
        for (const shared_ptr<TaskIndependentEvaluator> &evaluator : evals) {
            subfactories.push_back(
                make_shared<standard_scalar_open_list::TaskIndependentBestFirstOpenListFactory>(
                    evaluator, false));
            if (!preferred_evaluators.empty()) {
                subfactories.push_back(
                    make_shared<standard_scalar_open_list::TaskIndependentBestFirstOpenListFactory>(
                        evaluator, true));
            }
        }
        return make_shared<alternation_open_list::TaskIndependentAlternationOpenListFactory>(subfactories, boost);
    }
}


/*
  Helper function for creating a single g + w * h evaluator
  for weighted A*-style search.

  If w = 1, we do not introduce an unnecessary weighted evaluator:
  we use g + h instead of g + 1 * h.

  If w = 0, we omit the h-evaluator altogether:
  we use g instead of g + 0 * h.
*/

static shared_ptr<TaskIndependentEvaluator> create_task_independent_wastar_eval(
    const shared_ptr<g_evaluator::TaskIndependentGEvaluator> &g_eval,
    int w,
    const shared_ptr<TaskIndependentEvaluator> &h_eval,
    const string &name,
    const utils::Verbosity verbosity) {
    if (w == 0) {
        return g_eval;
    }
    shared_ptr<TaskIndependentEvaluator> w_h_eval = nullptr;
    if (w == 1) {
        w_h_eval = h_eval;
    } else {
        w_h_eval = make_shared<weighted_evaluator::TaskIndependentWeightedEvaluator>(h_eval, w, name + ".w_eval", verbosity);
    }
    return make_shared<sum_evaluator::TaskIndependentSumEvaluator>(
        vector<shared_ptr<TaskIndependentEvaluator>>({g_eval, w_h_eval}), name + ".sum_eval", verbosity);
}

shared_ptr<TaskIndependentOpenListFactory> create_task_independent_wastar_open_list_factory(
    const vector<shared_ptr<TaskIndependentEvaluator>> &evals,
    const vector<shared_ptr<TaskIndependentEvaluator>> &preferred,
    int boost,
    int weight,
    const string &name,
    const utils::Verbosity &verbosity) {
    shared_ptr<g_evaluator::TaskIndependentGEvaluator> g_eval =
        make_shared<g_evaluator::TaskIndependentGEvaluator>(name + ".g_eval", verbosity);
    vector<shared_ptr<TaskIndependentEvaluator>> f_evals;
    f_evals.reserve(evals.size());
    for (const shared_ptr<TaskIndependentEvaluator> &eval : evals)
        f_evals.push_back(create_task_independent_wastar_eval(g_eval, weight, eval, name, verbosity));

    return create_task_independent_alternation_open_list_factory_aux(
        f_evals,
        preferred,
        boost);
}


pair<shared_ptr<TaskIndependentOpenListFactory>, const shared_ptr<TaskIndependentEvaluator>>
create_task_independent_astar_open_list_factory_and_f_eval(const string &name,
                                                           const utils::Verbosity &verbosity,
                                                           const shared_ptr<TaskIndependentEvaluator> &eval) {
    shared_ptr<g_evaluator::TaskIndependentGEvaluator> g =
        make_shared<g_evaluator::TaskIndependentGEvaluator>(name + ".g_eval", verbosity);

    shared_ptr<TaskIndependentEvaluator> h = eval;

    shared_ptr<sum_evaluator::TaskIndependentSumEvaluator> f =
        make_shared<sum_evaluator::TaskIndependentSumEvaluator>(
            vector<shared_ptr<TaskIndependentEvaluator>>({g, h}),
            name + ".sum_eval",
            verbosity);

    vector<shared_ptr<TaskIndependentEvaluator>> evals = {f, h};

    shared_ptr<TaskIndependentOpenListFactory> open =
        make_shared<tiebreaking_open_list::TaskIndependentTieBreakingOpenListFactory>(evals, false, false);
    return make_pair(open, f);
}
}
