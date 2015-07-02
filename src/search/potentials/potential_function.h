#ifndef POTENTIALS_POTENTIAL_FUNCTION_H
#define POTENTIALS_POTENTIAL_FUNCTION_H

#include <vector>

class GlobalState;
class State;

namespace potentials {
class PotentialFunction {
    const std::vector<std::vector<double> > fact_potentials;

public:
    explicit PotentialFunction(
        const std::vector<std::vector<double> > &fact_potentials);
    ~PotentialFunction() = default;

    int get_value(const State &state) const;
    // TODO: Remove.
    int get_value(const GlobalState &global_state) const;
};

}

#endif
