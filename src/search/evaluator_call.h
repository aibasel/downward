#ifndef EVALUATOR_CALL_H
#define EVALUATOR_CALL_H

class EvaluationContext;
class EvaluationResult;
class Evaluator;
class State;

class EvaluatorCall {
    EvaluationContext &context;
    const State &state;
public:
    EvaluatorCall(EvaluationContext &context, const State &state);
    const EvaluationResult &operator[](Evaluator *evaluator);

    EvaluatorCall get_subcall(const State &state) const;
    const State &get_state() const;
    int get_g_value() const;
    bool get_calculate_preferred() const;
    bool is_preferred() const;
};

#endif
