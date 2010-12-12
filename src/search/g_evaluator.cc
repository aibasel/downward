#include "g_evaluator.h"
#include "option_parser.h"

GEvaluator::GEvaluator() {
}

GEvaluator::~GEvaluator() {
}

void GEvaluator::evaluate(int g, bool) {
    value = g;
}

bool GEvaluator::is_dead_end() const {
    return false;
}

bool GEvaluator::dead_end_is_reliable() const {
    return true;
}

int GEvaluator::get_value() const {
    return value;
}

static ScalarEvaluator *_parse(OptionParser &parser) {
    parser.parse();
    if (parser.dry_run)
        return 0;
    else
        return new GEvaluator;
}

static ScalarEvalPlugin _parse("g", _parse);
