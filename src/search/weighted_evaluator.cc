#include "weighted_evaluator.h"

#include <cstdlib>
#include <sstream>

#include "option_parser.h"

WeightedEvaluator::WeightedEvaluator(const Options &opts)
    : evaluator(opts.get_list<ScalarEvaluator *>("evals")[0]) 
      w(opts.get_list<ScalarEvaluator *>("weight")) {
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

static ScalarEvaluator *_parse(&OptionParser parser) {
    // create evaluator
    std::vector<ScalarEvaluator *> evals;
    parser.add_list_option<ScalarEvaluator *>("evals");
    parser.add_option<int>("weight");
    Options opts parser.parse();
    if (parser.dry_run)
        return 0;
    else
        return new WeightedEvaluator(opts);
}

static ScalarEvalPlugin _plugin("weight", _parse);
