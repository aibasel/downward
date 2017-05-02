#include "search_common.h"

#include "../open_list_factory.h"
#include "../option_parser_util.h"

#include "../evaluators/g_evaluator.h"
#include "../evaluators/sum_evaluator.h"
#include "../evaluators/weighted_evaluator.h"

#include "../open_lists/alternation_open_list.h"
#include "../open_lists/standard_scalar_open_list.h"
#include "../open_lists/tiebreaking_open_list.h"

#include <memory>

using namespace std;

namespace search_common {
using GEval = g_evaluator::GEvaluator;
using SumEval = sum_evaluator::SumEvaluator;
using WeightedEval = weighted_evaluator::WeightedEvaluator;

shared_ptr<OpenListFactory> create_standard_scalar_open_list_factory(
    Evaluator *eval, bool pref_only) {
    Options options;
    options.set("eval", eval);
    options.set("pref_only", pref_only);
    return make_shared<standard_scalar_open_list::StandardScalarOpenListFactory>(options);
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
    const vector<Evaluator *> &evals,
    const vector<Heuristic *> &preferred_heuristics,
    int boost) {
    if (evals.size() == 1 && preferred_heuristics.empty()) {
        return create_standard_scalar_open_list_factory(evals[0], false);
    } else {
        vector<shared_ptr<OpenListFactory>> subfactories;
        for (Evaluator *evaluator : evals) {
            subfactories.push_back(
                create_standard_scalar_open_list_factory(
                    evaluator, false));
            if (!preferred_heuristics.empty()) {
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
        options.get_list<Evaluator *>("evals"),
        options.get_list<Heuristic *>("preferred"),
        options.get<int>("boost"));
}

/*
  Helper function for creating a single g + w * h evaluator
  for weighted A*-style search.

  If w = 1, we do not introduce an unnecessary weighted evaluator:
  we use g + h instead of g + 1 * h.

  If w = 0, we omit the heuristic altogether:
  we use g instead of g + 0 * h.
*/
static Evaluator *create_wastar_eval(GEval *g_eval, int w, Evaluator *h_eval) {
    if (w == 0)
        return g_eval;
    Evaluator *w_h_eval = nullptr;
    if (w == 1)
        w_h_eval = h_eval;
    else
        w_h_eval = new WeightedEval(h_eval, w);
    return new SumEval(vector<Evaluator *>({g_eval, w_h_eval}));
}

shared_ptr<OpenListFactory> create_wastar_open_list_factory(
    const Options &options) {
    vector<Evaluator *> base_evals =
        options.get_list<Evaluator *>("evals");
    int w = options.get<int>("w");

    GEval *g_eval = new GEval();
    vector<Evaluator *> f_evals;
    f_evals.reserve(base_evals.size());
    for (Evaluator *eval : base_evals)
        f_evals.push_back(create_wastar_eval(g_eval, w, eval));

    return create_alternation_open_list_factory_aux(
        f_evals,
        options.get_list<Heuristic *>("preferred"),
        options.get<int>("boost"));
}

pair<shared_ptr<OpenListFactory>, Evaluator *>
create_astar_open_list_factory_and_f_eval(const Options &opts) {
    GEval *g = new GEval();
    Evaluator *h = opts.get<Evaluator *>("eval");
    Evaluator *f = new SumEval(vector<Evaluator *>({g, h}));
    vector<Evaluator *> evals = {f, h};

    Options options;
    options.set("evals", evals);
    options.set("pref_only", false);
    options.set("unsafe_pruning", false);
    shared_ptr<OpenListFactory> open =
        make_shared<tiebreaking_open_list::TieBreakingOpenListFactory>(options);
    return make_pair(open, f);
}
}
