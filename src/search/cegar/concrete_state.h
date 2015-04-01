#ifndef CEGAR_CONCRETE_STATE_H
#define CEGAR_CONCRETE_STATE_H

#include "utils.h"

#include "../state_id.h"

#include <cstddef>
#include <utility>
#include <vector>

class GlobalCondition;
class GlobalState;
class GlobalOperator;


namespace cegar {
class ConcreteState {
    GlobalState state;
    std::shared_ptr<StateRegistry> registry;
public:
    explicit ConcreteState(const GlobalState &state, std::shared_ptr<StateRegistry> registry);
    ~ConcreteState();
    //ConcreteState(const ConcreteState &other) = delete;
    ConcreteState& operator=(const ConcreteState &other);

    int operator[](std::size_t index) const;

    ConcreteState apply(const GlobalOperator &op) const;

    StateID get_id() const;
};

bool is_applicable(const GlobalOperator &op, const ConcreteState &state);
bool is_applicable(OperatorProxy op, const ConcreteState &state);

bool is_goal_state(TaskProxy task, const ConcreteState &state);

ConcreteState get_initial_state(TaskProxy task_proxy);

}
#endif
