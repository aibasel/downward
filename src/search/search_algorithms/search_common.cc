#include "search_common.h"

#include "../open_list_factory.h"

#include "../evaluators/g_evaluator.h"
#include "../evaluators/sum_evaluator.h"
#include "../evaluators/weighted_evaluator.h"
#include "../open_lists/alternation_open_list.h"
#include "../open_lists/best_first_open_list.h"
#include "../open_lists/tiebreaking_open_list.h"

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
static shared_ptr<OpenListFactory> create_alternation_open_list_factory_aux(
    const vector<shared_ptr<Evaluator>> &evals,
    const vector<shared_ptr<Evaluator>> &preferred_evaluators,
    int boost) {
    if (evals.size() == 1 && preferred_evaluators.empty()) {
        return make_shared<standard_scalar_open_list::BestFirstOpenListFactory>(
            evals[0], false);
    } else {
        vector<shared_ptr<OpenListFactory>> subfactories;
        for (const shared_ptr<Evaluator> &evaluator : evals) {
            subfactories.push_back(
                make_shared<standard_scalar_open_list::BestFirstOpenListFactory>(
                    evaluator, false));
            if (!preferred_evaluators.empty()) {
                subfactories.push_back(
                    make_shared<standard_scalar_open_list::BestFirstOpenListFactory>(
                        evaluator, true));
            }
        }
        return make_shared<alternation_open_list::AlternationOpenListFactory>(
            subfactories, boost);
    }
}

shared_ptr<OpenListFactory> create_greedy_open_list_factory(
    const vector<shared_ptr<Evaluator>> &evals,
    const vector<shared_ptr<Evaluator>> &preferred_evaluators,
    int boost) {
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
static shared_ptr<Evaluator> create_wastar_eval(
    utils::Verbosity verbosity, const shared_ptr<GEval> &g_eval,
    int weight, const shared_ptr<Evaluator> &h_eval) {
    if (weight == 0) {
        return g_eval;
    }
    shared_ptr<Evaluator> w_h_eval = nullptr;
    if (weight == 1) {
        w_h_eval = h_eval;
    } else {
        w_h_eval = make_shared<WeightedEval>(
            h_eval, weight, "wastar.w_h_eval", verbosity);
    }
    return make_shared<SumEval>(
        vector<shared_ptr<Evaluator>>({g_eval, w_h_eval}),
        "wastar.eval", verbosity);
}

shared_ptr<OpenListFactory> create_wastar_open_list_factory(
    const vector<shared_ptr<Evaluator>> &base_evals,
    const vector<shared_ptr<Evaluator>> &preferred, int boost,
    int weight, utils::Verbosity verbosity) {
    shared_ptr<GEval> g_eval = make_shared<GEval>(
        "wastar.g_eval", verbosity);
    vector<shared_ptr<Evaluator>> f_evals;
    f_evals.reserve(base_evals.size());
    for (const shared_ptr<Evaluator> &eval : base_evals)
        f_evals.push_back(create_wastar_eval(
                              verbosity, g_eval, weight, eval));

    return create_alternation_open_list_factory_aux(
        f_evals,
        preferred,
        boost);
}

pair<shared_ptr<OpenListFactory>, const shared_ptr<Evaluator>>
create_astar_open_list_factory_and_f_eval(
    const shared_ptr<Evaluator> &h_eval, utils::Verbosity verbosity
    ) {
    shared_ptr<GEval> g = make_shared<GEval>("astar.g_eval", verbosity);
    shared_ptr<Evaluator> f =
        make_shared<SumEval>(
            vector<shared_ptr<Evaluator>>({g, h_eval}),
            "astar.f_eval", verbosity);
    vector<shared_ptr<Evaluator>> evals = {f, h_eval};

    shared_ptr<OpenListFactory> open =
        make_shared<tiebreaking_open_list::TieBreakingOpenListFactory>(
            evals, false, false);
    return make_pair(open, f);
}
}
