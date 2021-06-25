#include "search_common.h"

#include "../open_list_factory.h"
#include "../operator_cost.h"
#include "../option_parser_util.h"

#include "../evaluators/g_evaluator.h"
#include "../evaluators/sum_evaluator.h"
#include "../evaluators/weighted_evaluator.h"
#include "../open_lists/alternation_open_list.h"
#include "../open_lists/best_first_open_list.h"
#include "../open_lists/tiebreaking_open_list.h"
#include "../tasks/cost_adapted_task.h"
#include "../tasks/root_task.h"

#include <memory>

using namespace std;

namespace search_common {
using GEval = g_evaluator::GEvaluator;
using SumEval = sum_evaluator::SumEvaluator;
using WeightedEval = weighted_evaluator::WeightedEvaluator;

void add_g_evaluator(options::Options &opts) {
    OperatorCost cost_type = opts.get<OperatorCost>("cost_type");

    shared_ptr<AbstractTask> task = tasks::g_root_task;
    if (cost_type != OperatorCost::NORMAL) {
        task = make_shared<tasks::CostAdaptedTask>(task, cost_type);
    }

    Options g_eval_opts;
    g_eval_opts.set<shared_ptr<AbstractTask>>("transform", task);
    g_eval_opts.set<bool>("cache_estimates", true);

    opts.set<shared_ptr<Evaluator>>(
        "g_evaluator", make_shared<g_evaluator::GEvaluator>(g_eval_opts));
}

void add_real_g_evaluator_if_needed(options::Options &opts) {
    OperatorCost cost_type = opts.get<OperatorCost>("cost_type");
    shared_ptr<Evaluator> g_evaluator = opts.get<shared_ptr<Evaluator>>(
        "g_evaluator", nullptr);

    shared_ptr<Evaluator> real_g_evaluator;
    if (opts.get<int>("bound") != numeric_limits<int>::max()) {
        if (g_evaluator && cost_type == OperatorCost::NORMAL) {
            real_g_evaluator = g_evaluator;
        } else {
            Options g_eval_opts;
            g_eval_opts.set<shared_ptr<AbstractTask>>("transform", tasks::g_root_task);
            g_eval_opts.set<bool>("cache_estimates", true);
            real_g_evaluator = make_shared<g_evaluator::GEvaluator>(g_eval_opts);
        }
        opts.set<shared_ptr<Evaluator>>("real_g_evaluator", real_g_evaluator);
    }
}

shared_ptr<OpenListFactory> create_standard_scalar_open_list_factory(
    const shared_ptr<Evaluator> &eval, bool pref_only) {
    Options options;
    options.set("eval", eval);
    options.set("pref_only", pref_only);
    return make_shared<standard_scalar_open_list::BestFirstOpenListFactory>(options);
}

static shared_ptr<OpenListFactory> create_alternation_open_list_factory(
    const vector<shared_ptr<OpenListFactory>> &subfactories, int boost) {
    Options options;
    options.set("sublists", subfactories);
    options.set("boost", boost);
    return make_shared<alternation_open_list::AlternationOpenListFactory>(options);
}

/*
  Helper function for common code of create_greedy_open_list_factory
  and create_wastar_open_list_factory.
*/
static shared_ptr<OpenListFactory> create_alternation_open_list_factory_aux(
    const vector<shared_ptr<Evaluator>> &evals,
    const vector<shared_ptr<Evaluator>> &preferred_evaluators,
    int boost) {
    if (evals.size() == 1 && preferred_evaluators.empty()) {
        return create_standard_scalar_open_list_factory(evals[0], false);
    } else {
        vector<shared_ptr<OpenListFactory>> subfactories;
        for (const shared_ptr<Evaluator> &evaluator : evals) {
            subfactories.push_back(
                create_standard_scalar_open_list_factory(
                    evaluator, false));
            if (!preferred_evaluators.empty()) {
                subfactories.push_back(
                    create_standard_scalar_open_list_factory(
                        evaluator, true));
            }
        }
        return create_alternation_open_list_factory(subfactories, boost);
    }
}

shared_ptr<OpenListFactory> create_greedy_open_list_factory(
    const Options &options) {
    return create_alternation_open_list_factory_aux(
        options.get_list<shared_ptr<Evaluator>>("evals"),
        options.get_list<shared_ptr<Evaluator>>("preferred"),
        options.get<int>("boost"));
}

/*
  Helper function for creating a single g + w * h evaluator
  for weighted A*-style search.

  If w = 1, we do not introduce an unnecessary weighted evaluator:
  we use g + h instead of g + 1 * h.

  If w = 0, we omit the h-evaluator altogether:
  we use g instead of g + 0 * h.
*/
static shared_ptr<Evaluator> create_wastar_eval(const shared_ptr<Evaluator> &g_eval, int w,
                                                const shared_ptr<Evaluator> &h_eval) {
    if (w == 0)
        return g_eval;
    shared_ptr<Evaluator> w_h_eval = nullptr;
    if (w == 1)
        w_h_eval = h_eval;
    else
        w_h_eval = make_shared<WeightedEval>(h_eval, w);
    return make_shared<SumEval>(vector<shared_ptr<Evaluator>>({g_eval, w_h_eval}));
}

shared_ptr<OpenListFactory> create_wastar_open_list_factory(
    const Options &options) {
    vector<shared_ptr<Evaluator>> base_evals =
        options.get_list<shared_ptr<Evaluator>>("evals");
    int w = options.get<int>("w");

    shared_ptr<Evaluator> g_eval = options.get<shared_ptr<Evaluator>>("g_evaluator");
    vector<shared_ptr<Evaluator>> f_evals;
    f_evals.reserve(base_evals.size());
    for (const shared_ptr<Evaluator> &eval : base_evals)
        f_evals.push_back(create_wastar_eval(g_eval, w, eval));

    return create_alternation_open_list_factory_aux(
        f_evals,
        options.get_list<shared_ptr<Evaluator>>("preferred"),
        options.get<int>("boost"));
}

pair<shared_ptr<OpenListFactory>, const shared_ptr<Evaluator>>
create_astar_open_list_factory_and_f_eval(const Options &opts) {
    shared_ptr<Evaluator> g = opts.get<shared_ptr<Evaluator>>("g_evaluator");
    shared_ptr<Evaluator> h = opts.get<shared_ptr<Evaluator>>("eval");
    shared_ptr<Evaluator> f = make_shared<SumEval>(vector<shared_ptr<Evaluator>>({g, h}));
    vector<shared_ptr<Evaluator>> evals = {f, h};

    Options options;
    options.set("evals", evals);
    options.set("pref_only", false);
    options.set("unsafe_pruning", false);
    shared_ptr<OpenListFactory> open =
        make_shared<tiebreaking_open_list::TieBreakingOpenListFactory>(options);
    return make_pair(open, f);
}
}
