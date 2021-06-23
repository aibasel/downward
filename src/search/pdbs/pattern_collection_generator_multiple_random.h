#ifndef PDBS_PATTERN_COLLECTION_GENERATOR_MULTIPLE_RANDOM_H
#define PDBS_PATTERN_COLLECTION_GENERATOR_MULTIPLE_RANDOM_H

#include "pattern_collection_generator_multiple.h"

namespace pdbs {
class PatternCollectionGeneratorMultipleRandom : public PatternCollectionGeneratorMultiple {
    // Options for the RCG algorithm.
    const int max_pdb_size;
    const double random_causal_graph_max_time;
    bool bidirectional;

    std::vector<std::vector<int>> cg_neighbors;
protected:
    virtual std::string get_name() const override;
    virtual void initialize(const std::shared_ptr<AbstractTask> &task) override;
    virtual PatternInformation compute_pattern(
        const std::shared_ptr<AbstractTask> &task,
        FactPair goal,
        std::unordered_set<int> &&blacklisted_variables,
        const utils::CountdownTimer &timer,
        int remaining_collection_size) override;
public:
    explicit PatternCollectionGeneratorMultipleRandom(options::Options &opts);
};
}

#endif
