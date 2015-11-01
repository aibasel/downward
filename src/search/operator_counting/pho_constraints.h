#ifndef OPERATOR_COUNTING_PHO_CONSTRAINTS_H
#define OPERATOR_COUNTING_PHO_CONSTRAINTS_H

#include "constraint_generator.h"

#include "../option_parser.h"

#include <memory>
#include <vector>

class CanonicalPDBsHeuristic;
class PatternDatabase;

namespace OperatorCounting {
class PhOConstraints : public ConstraintGenerator {
    int constraint_offset;

    // We store the options until we get the task in the initialize function.
    Options options;
    // We can use the PDBs from a canonical heuristic.
    std::unique_ptr<CanonicalPDBsHeuristic> pdb_source;
    std::vector<PatternDatabase *> pdbs;
    void generate_pdbs(const std::shared_ptr<AbstractTask> task);
public:
    explicit PhOConstraints(const Options &opts);
    ~PhOConstraints();

    virtual void initialize_constraints(
        const std::shared_ptr<AbstractTask> task,
        std::vector<LPConstraint> &constraints,
        double infinity) override;
    virtual bool update_constraints(
        const State &state, LPSolver &lp_solver) override;
};
}

#endif
