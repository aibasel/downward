#ifndef CEGAR_CONCRETE_STATE_H
#define CEGAR_CONCRETE_STATE_H

class GlobalState;


class ConcreteState {
    const GlobalState &state;
public:
    ConcreteState(const GlobalState &state);
    ~ConcreteState();
};

#endif
