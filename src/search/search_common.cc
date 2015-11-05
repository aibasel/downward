#include "search_common.h"

#include "g_evaluator.h"
#include "open_lists/alternation_open_list.h"
#include "open_lists/open_list_factory.h"
#include "open_lists/standard_scalar_open_list.h"
#include "open_lists/tiebreaking_open_list.h"
#include "option_parser_util.h"
#include "sum_evaluator.h"

#include <memory>

using namespace std;


shared_ptr<OpenListFactory> create_standard_scalar_open_list_factory(
    ScalarEvaluator *eval, bool pref_only) {
    Options options;
    options.set("eval", eval);
    options.set("pref_only", pref_only);
    return make_shared<StandardScalarOpenListFactory>(options);
}

static shared_ptr<OpenListFactory> create_alternation_open_list_factory(
    const vector<shared_ptr<OpenListFactory> > &subfactories, int boost) {
    Options options;
    options.set("sublists", subfactories);
    options.set("boost", boost);
    return make_shared<AlternationOpenListFactory>(options);
}

shared_ptr<OpenListFactory> create_greedy_open_list_factory(
    const Options &options) {
    vector<ScalarEvaluator *> evals =
        options.get_list<ScalarEvaluator *>("evals");
    vector<Heuristic *> preferred_list =
        options.get_list<Heuristic *>("preferred");
    int boost = options.get<int>("boost");

    if (evals.size() == 1 && preferred_list.empty()) {
        return create_standard_scalar_open_list_factory(evals[0], false);
    } else {
        vector<shared_ptr<OpenListFactory> > subfactories;
        for (ScalarEvaluator *evaluator : evals) {
            subfactories.push_back(
                create_standard_scalar_open_list_factory(
                    evaluator, false));
            if (!preferred_list.empty()) {
                subfactories.push_back(
                    create_standard_scalar_open_list_factory(
                        evaluator, true));
            }
        }
        return create_alternation_open_list_factory(subfactories, boost);
    }
}

pair<shared_ptr<OpenListFactory>, ScalarEvaluator *>
create_astar_open_list_factory_and_f_eval(const Options &opts) {
    GEvaluator *g = new GEvaluator();
    ScalarEvaluator *h = opts.get<ScalarEvaluator *>("eval");
    ScalarEvaluator *f = new SumEvaluator({g, h});
    vector<ScalarEvaluator *> evals {f, h};

    Options options;
    options.set("evals", evals);
    options.set("pref_only", false);
    options.set("unsafe_pruning", false);
    shared_ptr<OpenListFactory> open =
        make_shared<TieBreakingOpenListFactory>(options);
    return make_pair(open, f);
}
