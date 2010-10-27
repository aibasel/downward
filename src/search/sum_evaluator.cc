#include "sum_evaluator.h"

#include <limits>

#include "option_parser.h"

SumEvaluator::SumEvaluator(const std::vector<ScalarEvaluator *> &evals)
    : evaluators(evals) {
}

SumEvaluator::~SumEvaluator() {
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

void SumEvaluator::get_involved_heuristics(std::set<Heuristic *> &hset) {
    for (unsigned int i = 0; i < evaluators.size(); i++)
        evaluators[i]->get_involved_heuristics(hset);
}

ScalarEvaluator *SumEvaluator::create(const std::vector<std::string> &config,
                                      int start, int &end, bool dry_run) {
    if (config[start + 1] != "(")
        throw ParseError(start + 1);

    // create evaluators
    std::vector<ScalarEvaluator *> evals;
    OptionParser::instance()->parse_scalar_evaluator_list(config, start + 2,
                                                          end, false, evals,
                                                          dry_run);

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
