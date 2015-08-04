#include "search_common.h"

#include "open_lists/alternation_open_list.h"
#include "open_lists/open_list_factory.h"
#include "open_lists/standard_scalar_open_list.h"
#include "option_parser_util.h"

#include <memory>

using namespace std;


static shared_ptr<OpenListFactory> create_standard_scalar_open_list_factory(
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
