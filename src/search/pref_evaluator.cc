#include "pref_evaluator.h"
#include "option_parser.h"

PrefEvaluator::PrefEvaluator() {
}

PrefEvaluator::~PrefEvaluator() {
}

void PrefEvaluator::evaluate(int, bool preferred) {
    value_preferred = preferred;
}

bool PrefEvaluator::is_dead_end() const {
    return false;
}

bool PrefEvaluator::dead_end_is_reliable() const {
    return true;
}

int PrefEvaluator::get_value() const {
    if (value_preferred)
        return 0;
    else
        return 1;
}

ScalarEvaluator *PrefEvaluator::create(const std::vector<std::string> &config,
                                       int start, int &end, bool dry_run) {
    OptionParser::instance()->set_end_for_simple_config(config, start, end);
    if (dry_run)
        return 0;
    else
        return new PrefEvaluator;
}
