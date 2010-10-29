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

ScalarEvaluator *GEvaluator::create(const std::vector<std::string> &config,
                                    int start, int &end, bool dry_run) {
    OptionParser::instance()->set_end_for_simple_config(config, start, end);
    if (dry_run)
        return 0;
    else
        return new GEvaluator;
}
