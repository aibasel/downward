#ifndef POTENTIALS_POTENTIAL_HEURISTICS_H
#define POTENTIALS_POTENTIAL_HEURISTICS_H

#include "../heuristic.h"

#include <memory>
#include <vector>


namespace potentials {
class PotentialFunction;

class PotentialHeuristics : public Heuristic {
    std::vector<std::shared_ptr<PotentialFunction> > functions;

protected:
    virtual void initialize() override;
    virtual int compute_heuristic(const GlobalState &global_state) override;

public:
    explicit PotentialHeuristics(const Options &opts);
    ~PotentialHeuristics() = default;
};
}

#endif
