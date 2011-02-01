#include "combining_evaluator.h"

#include <limits>

using namespace std;

CombiningEvaluator::CombiningEvaluator(
    const vector<ScalarEvaluator *> &subevaluators_)
    : subevaluators(subevaluators_),
      subevaluator_values(subevaluators_.size()) {
}

CombiningEvaluator::~CombiningEvaluator() {
}

void CombiningEvaluator::evaluate(int g, bool preferred) {
    dead_end = false;
    dead_end_reliable = false;
    for (size_t i = 0; i < subevaluators.size(); ++i) {
        subevaluators[i]->evaluate(g, preferred);
        subevaluator_values[i] = subevaluators[i]->get_value();
        if (subevaluators[i]->is_dead_end()) {
            dead_end = true;
            if (subevaluators[i]->dead_end_is_reliable())
                dead_end_reliable = true;
        }
    }

    if (dead_end)
        value = numeric_limits<int>::max();
    else
        value = combine_values(subevaluator_values);
}

bool CombiningEvaluator::is_dead_end() const {
    return dead_end;
}

bool CombiningEvaluator::dead_end_is_reliable() const {
    return dead_end_reliable;
}

int CombiningEvaluator::get_value() const {
    return value;
}

void CombiningEvaluator::get_involved_heuristics(std::set<Heuristic *> &hset) {
    for (unsigned int i = 0; i < subevaluators.size(); i++)
        subevaluators[i]->get_involved_heuristics(hset);
}
