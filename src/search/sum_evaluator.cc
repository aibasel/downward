#include "sum_evaluator.h"

#include "option_parser.h"
#include "plugin.h"

#include <cassert>

using namespace std;


SumEvaluator::SumEvaluator(const vector<ScalarEvaluator *> &subevaluators)
    : CombiningEvaluator(subevaluators) {
}

SumEvaluator::~SumEvaluator() {
}

int SumEvaluator::combine_values(const vector<int> &values) {
    int result = 0;
    for (size_t i = 0; i < values.size(); ++i) {
        assert(values[i] >= 0);
        result += values[i];
        assert(result >= 0); // Check against overflow.
    }
    return result;
}

static ScalarEvaluator *create(const vector<string> &config,
                               int start, int &end, bool dry_run) {
    if (config[start + 1] != "(")
        throw ParseError(start + 1);

    // create evaluators
    vector<ScalarEvaluator *> evals;
    OptionParser::instance()->parse_scalar_evaluator_list(
        config, start + 2, end, false, evals, dry_run);

    if (evals.empty())
        throw ParseError(end);
    // need at least one evaluator

    end++;
    if (config[end] != ")")
        throw ParseError(end);

    if (dry_run)
        return 0;
    else
        return new SumEvaluator(evals);
}

static ScalarEvaluatorPlugin plugin("sum", create);
