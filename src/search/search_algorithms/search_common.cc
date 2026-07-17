#include "search_common.h"

#include "../evaluators/g_evaluator.h"
#include "../evaluators/sum_evaluator.h"
#include "../evaluators/weighted_evaluator.h"
#include "../open_lists/alternation_open_list.h"
#include "../open_lists/best_first_open_list.h"
#include "../open_lists/tiebreaking_open_list.h"
#include "../utils/component_errors.h"

#include <memory>

using namespace std;

namespace search_common {
using GEval = g_evaluator::GEvaluator;
using SumEval = sum_evaluator::SumEvaluator;
using WeightedEval = weighted_evaluator::WeightedEvaluator;

/*
  Helper function for common code of create_greedy_open_list_factory
  and create_wastar_open_list_factory.
*/
static shared_ptr<TaskIndependentOpenListFactory>
create_alternation_open_list_factory_aux(
    const vector<shared_ptr<TaskIndependentEvaluator>> &evals,
    const vector<shared_ptr<TaskIndependentEvaluator>> &preferred_evaluators,
    int boost) {
    if (evals.size() == 1 && preferred_evaluators.empty()) {
        return components::make_auto_task_independent_component<
            standard_scalar_open_list::BestFirstOpenListFactory,
            OpenListFactory>(evals[0], false);
    } else {
        vector<shared_ptr<TaskIndependentOpenListFactory>> subfactories;
        for (const shared_ptr<TaskIndependentEvaluator> &evaluator : evals) {
            subfactories.push_back(
                components::make_auto_task_independent_component<
                    standard_scalar_open_list::BestFirstOpenListFactory,
                    OpenListFactory>(evaluator, false));
            if (!preferred_evaluators.empty()) {
                subfactories.push_back(
                    components::make_auto_task_independent_component<
                        standard_scalar_open_list::BestFirstOpenListFactory,
                        OpenListFactory>(evaluator, true));
            }
        }
        return components::make_auto_task_independent_component<
            alternation_open_list::AlternationOpenListFactory, OpenListFactory>(
            subfactories, boost);
    }
}

shared_ptr<TaskIndependentOpenListFactory> create_greedy_open_list_factory(
    const vector<shared_ptr<TaskIndependentEvaluator>> &evals,
    const vector<shared_ptr<TaskIndependentEvaluator>> &preferred_evaluators,
    int boost) {
    utils::verify_list_not_empty(evals, "evals");
    return create_alternation_open_list_factory_aux(
        evals, preferred_evaluators, boost);
}

/*
  Helper function for creating a single g + w * h evaluator
  for weighted A*-style search.

  If w = 1, we do not introduce an unnecessary weighted evaluator:
  we use g + h instead of g + 1 * h.

  If w = 0, we omit the h-evaluator altogether:
  we use g instead of g + 0 * h.
*/
static shared_ptr<TaskIndependentEvaluator> create_wastar_eval(
    utils::Verbosity verbosity,
    const shared_ptr<TaskIndependentEvaluator> &g_eval, int weight,
    const shared_ptr<TaskIndependentEvaluator> &h_eval) {
    if (weight == 0) {
        return g_eval;
    }
    shared_ptr<TaskIndependentEvaluator> w_h_eval = nullptr;
    if (weight == 1) {
        w_h_eval = h_eval;
    } else {
        w_h_eval = components::make_auto_task_independent_component<
            WeightedEval, Evaluator>(
            h_eval, weight, "wastar.w_h_eval", verbosity);
    }
    return components::make_auto_task_independent_component<SumEval, Evaluator>(
        vector<shared_ptr<TaskIndependentEvaluator>>({g_eval, w_h_eval}),
        "wastar.eval", verbosity);
}

shared_ptr<TaskIndependentOpenListFactory> create_wastar_open_list_factory(
    const vector<shared_ptr<TaskIndependentEvaluator>> &evals,
    const vector<shared_ptr<TaskIndependentEvaluator>> &preferred, int boost,
    int weight, utils::Verbosity verbosity) {
    utils::verify_list_not_empty(evals, "evals");
    shared_ptr<TaskIndependentEvaluator> g_eval =
        components::make_auto_task_independent_component<GEval, Evaluator>(
            "wastar.g_eval", verbosity);
    vector<shared_ptr<TaskIndependentEvaluator>> f_evals;
    f_evals.reserve(evals.size());
    for (const shared_ptr<TaskIndependentEvaluator> &eval : evals)
        f_evals.push_back(create_wastar_eval(verbosity, g_eval, weight, eval));

    return create_alternation_open_list_factory_aux(f_evals, preferred, boost);
}

pair<
    shared_ptr<TaskIndependentOpenListFactory>,
    const shared_ptr<TaskIndependentEvaluator>>
create_astar_open_list_factory_and_f_eval(
    const shared_ptr<TaskIndependentEvaluator> &h_eval,
    utils::Verbosity verbosity) {
    shared_ptr<TaskIndependentEvaluator> g =
        components::make_auto_task_independent_component<GEval, Evaluator>(
            "astar.g_eval", verbosity);
    shared_ptr<TaskIndependentEvaluator> f =
        components::make_auto_task_independent_component<SumEval, Evaluator>(
            vector<shared_ptr<TaskIndependentEvaluator>>({g, h_eval}),
            "astar.f_eval", verbosity);
    vector<shared_ptr<TaskIndependentEvaluator>> evals = {f, h_eval};

    shared_ptr<TaskIndependentOpenListFactory> open =
        components::make_auto_task_independent_component<
            tiebreaking_open_list::TieBreakingOpenListFactory, OpenListFactory>(
            evals, false, false);
    return make_pair(open, f);
}
}
