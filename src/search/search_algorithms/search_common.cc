#include "search_common.h"

#include "../open_list_factory.h"

#include "../evaluators/g_evaluator.h"
#include "../evaluators/sum_evaluator.h"
#include "../evaluators/weighted_evaluator.h"
//issue559//#include "../open_lists/alternation_open_list.h"
//issue559//#include "../open_lists/best_first_open_list.h"
#include "../open_lists/tiebreaking_open_list.h"
#include "../utils/component_errors.h"

#include <memory>

using namespace std;

namespace search_common {
using GEval = g_evaluator::TaskIndependentGEvaluator;
using SumEval = sum_evaluator::TaskIndependentSumEvaluator;
using WeightedEval = weighted_evaluator::TaskIndependentWeightedEvaluator;

//issue559///*
//issue559//  Helper function for common code of create_greedy_open_list_factory
//issue559//  and create_wastar_open_list_factory.
//issue559//*/
//issue559//static shared_ptr<OpenListFactory> create_alternation_open_list_factory_aux(
//issue559//    const vector<shared_ptr<Evaluator>> &evals,
//issue559//    const vector<shared_ptr<Evaluator>> &preferred_evaluators,
//issue559//    int boost) {
//issue559//    if (evals.size() == 1 && preferred_evaluators.empty()) {
//issue559//        return make_shared<standard_scalar_open_list::BestFirstOpenListFactory>(
//issue559//            evals[0], false);
//issue559//    } else {
//issue559//        vector<shared_ptr<OpenListFactory>> subfactories;
//issue559//        for (const shared_ptr<Evaluator> &evaluator : evals) {
//issue559//            subfactories.push_back(
//issue559//                make_shared<standard_scalar_open_list::BestFirstOpenListFactory>(
//issue559//                    evaluator, false));
//issue559//            if (!preferred_evaluators.empty()) {
//issue559//                subfactories.push_back(
//issue559//                    make_shared<standard_scalar_open_list::BestFirstOpenListFactory>(
//issue559//                        evaluator, true));
//issue559//            }
//issue559//        }
//issue559//        return make_shared<alternation_open_list::AlternationOpenListFactory>(
//issue559//            subfactories, boost);
//issue559//    }
//issue559//}
//issue559//
//issue559//shared_ptr<OpenListFactory> create_greedy_open_list_factory(
//issue559//    const vector<shared_ptr<Evaluator>> &evals,
//issue559//    const vector<shared_ptr<Evaluator>> &preferred_evaluators,
//issue559//    int boost) {
//issue559//    utils::verify_list_not_empty(evals, "evals");
//issue559//    return create_alternation_open_list_factory_aux(
//issue559//        evals, preferred_evaluators, boost);
//issue559//}
//issue559//
//issue559///*
//issue559//  Helper function for creating a single g + w * h evaluator
//issue559//  for weighted A*-style search.
//issue559//
//issue559//  If w = 1, we do not introduce an unnecessary weighted evaluator:
//issue559//  we use g + h instead of g + 1 * h.
//issue559//
//issue559//  If w = 0, we omit the h-evaluator altogether:
//issue559//  we use g instead of g + 0 * h.
//issue559//*/
//issue559//static shared_ptr<Evaluator> create_wastar_eval(
//issue559//    utils::Verbosity verbosity, const shared_ptr<GEval> &g_eval,
//issue559//    int weight, const shared_ptr<Evaluator> &h_eval) {
//issue559//    if (weight == 0) {
//issue559//        return g_eval;
//issue559//    }
//issue559//    shared_ptr<Evaluator> w_h_eval = nullptr;
//issue559//    if (weight == 1) {
//issue559//        w_h_eval = h_eval;
//issue559//    } else {
//issue559//        w_h_eval = make_shared<WeightedEval>(
//issue559//            h_eval, weight, "wastar.w_h_eval", verbosity);
//issue559//    }
//issue559//    return make_shared<SumEval>(
//issue559//        vector<shared_ptr<Evaluator>>({g_eval, w_h_eval}),
//issue559//        "wastar.eval", verbosity);
//issue559//}
//issue559//
//issue559//shared_ptr<OpenListFactory> create_wastar_open_list_factory(
//issue559//    const vector<shared_ptr<Evaluator>> &evals,
//issue559//    const vector<shared_ptr<Evaluator>> &preferred, int boost,
//issue559//    int weight, utils::Verbosity verbosity) {
//issue559//    utils::verify_list_not_empty(evals, "evals");
//issue559//    shared_ptr<GEval> g_eval = make_shared<GEval>(
//issue559//        "wastar.g_eval", verbosity);
//issue559//    vector<shared_ptr<Evaluator>> f_evals;
//issue559//    f_evals.reserve(evals.size());
//issue559//    for (const shared_ptr<Evaluator> &eval : evals)
//issue559//        f_evals.push_back(create_wastar_eval(
//issue559//                              verbosity, g_eval, weight, eval));
//issue559//
//issue559//    return create_alternation_open_list_factory_aux(
//issue559//        f_evals,
//issue559//        preferred,
//issue559//        boost);
//issue559//}

pair<shared_ptr<TaskIndependentOpenListFactory>, const shared_ptr<TaskIndependentEvaluator>>
create_task_independent_astar_open_list_factory_and_f_eval(
    const shared_ptr<TaskIndependentEvaluator> &h_eval, const string &description, utils::Verbosity verbosity
    ) {
    shared_ptr<GEval> g = make_shared<GEval>(description + ".g_eval", verbosity);
    shared_ptr<TaskIndependentEvaluator> f =
        make_shared<SumEval>(
            vector<shared_ptr<TaskIndependentEvaluator>>({g, h_eval}),
            description + ".f_eval", verbosity);
    vector<shared_ptr<TaskIndependentEvaluator>> evals = {f, h_eval};

    shared_ptr<TaskIndependentOpenListFactory> open =
        make_shared<tiebreaking_open_list::TaskIndependentTieBreakingOpenListFactory>(
            evals, false, false);
    return make_pair(open, f);
}
}
