#ifndef POTENTIALS_POTENTIAL_FUNCTION_H
#define POTENTIALS_POTENTIAL_FUNCTION_H

#include <vector>

class State;


namespace potentials {
/*
  A potential function calculates the sum of potentials in a given state.

  We decouple potential functions from potential heuristics to avoid the
  overhead that is induced by evaluating heuristics whenever possible.
*/
class PotentialFunction {
    const std::vector<std::vector<double> > fact_potentials;

public:
    explicit PotentialFunction(
        const std::vector<std::vector<double> > &fact_potentials);
    ~PotentialFunction() = default;

    int get_value(const State &state) const;
};
}

#endif
