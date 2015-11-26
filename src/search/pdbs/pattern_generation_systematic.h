#ifndef PDBS_PATTERN_GENERATION_SYSTEMATIC_H
#define PDBS_PATTERN_GENERATION_SYSTEMATIC_H

#include "pattern_generator.h"

#include "../utilities.h"
#include "../task_tools.h"

#include <memory>
#include <unordered_set>
#include <vector>

class CanonicalPDBsHeuristic;
class CausalGraph;
class OptionParser;
class Options;


// Invariant: patterns are always sorted.
class PatternGenerationSystematic : public PatternCollectionGenerator {
    using PatternSet = std::unordered_set<Pattern>;

    const size_t max_pattern_size;
    const bool only_interesting_patterns;
    std::shared_ptr<Patterns> patterns;
    PatternSet pattern_set;  // Cleared after pattern computation.

    void enqueue_pattern_if_new(const Pattern &pattern);
    void compute_eff_pre_neighbors(const CausalGraph &cg,
                                   const Pattern &pattern,
                                   std::vector<int> &result) const;
    void compute_connection_points(const CausalGraph &cg,
                                   const Pattern &pattern,
                                   std::vector<int> &result) const;

    void build_sga_patterns(TaskProxy task_proxy, const CausalGraph &cg);
    void build_patterns(TaskProxy task_proxy);
    void build_patterns_naive(TaskProxy task_proxy);
public:
    explicit PatternGenerationSystematic(const Options &opts);
    ~PatternGenerationSystematic() = default;

    virtual PatternCollection generate(std::shared_ptr<AbstractTask> task) override;
};

#endif
