#include "weighted_evaluator.h"

WeightedEvaluator::WeightedEvaluator(ScalarEvaluator *eval, int weight) 
    : evaluator(eval), w(weight) {
}

WeightedEvaluator::~WeightedEvaluator(){
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


