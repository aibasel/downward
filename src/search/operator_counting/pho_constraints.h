#ifndef OPERATOR_COUNTING_PHO_CONSTRAINTS_H
#define OPERATOR_COUNTING_PHO_CONSTRAINTS_H

#include "constraint_generator.h"

#include "../pdbs/types.h"

#include <memory>

class CanonicalPDBsHeuristic;
class Options;
class PatternCollectionGenerator;

namespace OperatorCounting {
class PhOConstraints : public ConstraintGenerator {
    std::shared_ptr<PatternCollectionGenerator> pattern_generator;

    int constraint_offset;
    std::shared_ptr<PDBCollection> pdbs;
public:
    explicit PhOConstraints(const Options &opts);
    ~PhOConstraints() = default;

    virtual void initialize_constraints(
        const std::shared_ptr<AbstractTask> task,
        std::vector<LPConstraint> &constraints,
        double infinity) override;
    virtual bool update_constraints(
        const State &state, LPSolver &lp_solver) override;
};
}

#endif
