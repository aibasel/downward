#ifndef OPERATOR_COUNTING_PHO_CONSTRAINTS_H
#define OPERATOR_COUNTING_PHO_CONSTRAINTS_H

#include "constraint_generator.h"

#include "../pdbs/types.h"

#include <memory>

class Options;

namespace PDBs {
class PatternCollectionGenerator;
}

namespace OperatorCounting {
class PhOConstraints : public ConstraintGenerator {
    std::shared_ptr<PDBs::PatternCollectionGenerator> pattern_generator;

    int constraint_offset;
    std::shared_ptr<PDBs::PDBCollection> pdbs;
public:
    explicit PhOConstraints(const Options &opts);
    ~PhOConstraints() = default;

    virtual void initialize_constraints(
        const std::shared_ptr<AbstractTask> task,
        std::vector<LP::LPConstraint> &constraints,
        double infinity) override;
    virtual bool update_constraints(
        const State &state, LP::LPSolver &lp_solver) override;
};
}

#endif
