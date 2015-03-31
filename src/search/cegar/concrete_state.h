#ifndef CEGAR_CONCRETE_STATE_H
#define CEGAR_CONCRETE_STATE_H

#include <cstddef>

class GlobalCondition;
class GlobalState;
class GlobalOperator;


namespace cegar {
class ConcreteState {
    const GlobalState &state;
public:
    ConcreteState(const GlobalState &state);
    ~ConcreteState();

    int operator[](std::size_t index) const;
};

bool is_applicable(const GlobalCondition &condition, const ConcreteState &state);
bool is_applicable(const GlobalOperator &op, const ConcreteState &state);
}
#endif
