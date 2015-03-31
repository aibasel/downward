#ifndef CEGAR_CONCRETE_STATE_H
#define CEGAR_CONCRETE_STATE_H

#include "../state_id.h"

#include <cstddef>
#include <utility>
#include <vector>

class GlobalCondition;
class GlobalState;
class GlobalOperator;


namespace cegar {
class ConcreteState {
    const GlobalState *state;
public:
    explicit ConcreteState(const GlobalState &state);
    ~ConcreteState();
    //ConcreteState(const ConcreteState &other) = delete;

    int operator[](std::size_t index) const;

    ConcreteState apply(const GlobalOperator &op) const;

    StateID get_id() const;
};

bool is_applicable(const GlobalCondition &condition, const ConcreteState &state);
bool is_applicable(const GlobalOperator &op, const ConcreteState &state);

// TODO: Use GoalsProxy.
bool is_goal_state(std::vector<std::pair<int, int> > goals, const ConcreteState &state);

}
#endif
