#ifndef CEGAR_CONCRETE_STATE_H
#define CEGAR_CONCRETE_STATE_H

#include <cstddef>

class GlobalState;


class ConcreteState {
    const GlobalState &state;
public:
    ConcreteState(const GlobalState &state);
    ~ConcreteState();

    int operator[](std::size_t index) const;
};

#endif
