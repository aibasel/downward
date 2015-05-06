#ifndef EAGER_EVALUATION_CONTEXT_H
#define EAGER_EVALUATION_CONTEXT_H

#include "evaluation_context.h"
#include "global_state.h"

class SearchStatistics;

class EagerEvaluationContext : public EvaluationContext {
    GlobalState state;
    int g_value;
    bool preferred;
    SearchStatistics *statistics;

public:
    // Use "statistics = nullptr" if no statistics are desired.
    EagerEvaluationContext(
        const GlobalState &state, int g_value, bool is_preferred,
        SearchStatistics *statistics);

    /*
      Use the following constructor when you don't care about g
      values, preferredness or statistics, e.g. when sampling states
      for heuristics.
    */
    explicit EagerEvaluationContext(const GlobalState &state);
    ~EagerEvaluationContext() = default;

    virtual const EvaluationResult &get_result(ScalarEvaluator *heur) override;

    virtual const GlobalState &get_state() const override;

    virtual int get_g_value() const override;
    virtual bool is_preferred() const override;
};

#endif
