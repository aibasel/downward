#include "sum_evaluator.h"

#include <limits>

SumEvaluator::SumEvaluator() {
}

SumEvaluator::~SumEvaluator(){
}

void SumEvaluator::add_evaluator(ScalarEvaluator *eval) {
    evaluators.push_back(eval);
}

void SumEvaluator::evaluate(int g, bool preferred) {
    dead_end = false;
    dead_end_reliable = false;
    value = 0;
    for (unsigned int i = 0; i < evaluators.size(); i++) {
        evaluators[i]->evaluate(g, preferred);

        // add value
        // TODO: test for overflow
        value += evaluators[i]->get_value();

        // check for dead end
        if (evaluators[i]->is_dead_end()) {
			value = std::numeric_limits<int>::max();
            dead_end = true;
            if (evaluators[i]->dead_end_is_reliable()) {
                dead_end_reliable = true;
            }
        }
    }
}

bool SumEvaluator::is_dead_end() const {
    return dead_end;
}

bool SumEvaluator::dead_end_is_reliable() const {
    return dead_end_reliable;
}

int SumEvaluator::get_value() const {
    return value;
}


