#include "weighted_evaluator.h"

#include <cstdlib>
#include <sstream>

#include "option_parser.h"

WeightedEvaluator::WeightedEvaluator(ScalarEvaluator *eval, int weight)
    : evaluator(eval), w(weight) {
}

WeightedEvaluator::~WeightedEvaluator() {
}

void WeightedEvaluator::evaluate(int g, bool preferred) {
    evaluator->evaluate(g, preferred);
    value = w * evaluator->get_value();
    // TODO: catch overflow?
}

bool WeightedEvaluator::is_dead_end() const {
    return evaluator->is_dead_end();
}

bool WeightedEvaluator::dead_end_is_reliable() const {
    return evaluator->dead_end_is_reliable();
}

int WeightedEvaluator::get_value() const {
    return value;
}

void WeightedEvaluator::get_involved_heuristics(std::set<Heuristic *> &hset) {
    evaluator->get_involved_heuristics(hset);
}

ScalarEvaluator *WeightedEvaluator::create(
    const std::vector<std::string> &config, int start, int &end, bool dry_run) {
    if (config[start + 1] != "(")
        throw ParseError(start + 1);

    // create evaluator
    std::vector<ScalarEvaluator *> evals;
    OptionParser *parser = OptionParser::instance();
    parser->parse_scalar_evaluator_list(config, start + 2, end,
                                        true, evals, dry_run);

    end++;  // on comma
    end++;  // on weight

    int weight = parser->parse_int(config, end, end);
    end++;
    if (config[end] != ")")
        throw ParseError(end);

    if (dry_run)
        return 0;
    else
        return new WeightedEvaluator(evals[0], weight);
}
