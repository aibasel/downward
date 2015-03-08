#include "max_evaluator.h"

#include "option_parser.h"
#include "plugin.h"

#include <cassert>

using namespace std;


MaxEvaluator::MaxEvaluator(const vector<ScalarEvaluator *> &subevaluators)
    : CombiningEvaluator(subevaluators) {
}

MaxEvaluator::~MaxEvaluator() {
}

int MaxEvaluator::combine_values(const vector<int> &values) {
    int result = 0;
    for (int value : values) {
        assert(value >= 0);
        result = max(result, value);
    }
    return result;
}

/* commented out to silence compiler warning while this is unused.

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

    ++end;
    if (config[end] != ")")
        throw ParseError(end);

    if (dry_run)
        return 0;
    else
        return new MaxEvaluator(evals);
}
*/

// TODO: Comment this in once the max/sum evaluator stuff is fixed.
//       For now, it's commented out to use the IPC implementation of
//       max again, see issue181.
// static Plugin<ScalarEvaluator> plugin("max", create);
