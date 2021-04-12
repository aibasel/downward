#ifndef PDBS_PATTERN_COLLECTION_GENERATOR_MULTIPLE_CEGAR_H
#define PDBS_PATTERN_COLLECTION_GENERATOR_MULTIPLE_CEGAR_H

#include "pattern_generator.h"

namespace options {
class Options;
}

namespace utils {
class CountdownTimer;
class RandomNumberGenerator;
enum class Verbosity;
}

namespace pdbs {
class PatternCollectionGeneratorMultipleCegar : public PatternCollectionGenerator {
    // Options for the CEGAR algorithm.
    const int max_pdb_size;
    const int max_collection_size;
    const bool wildcard_plans;
    const double cegar_max_time;

    // Options for this generator.
    const utils::Verbosity verbosity;
    std::shared_ptr<utils::RandomNumberGenerator> rng;
    // We store the random seed for creating different RNG objects for CEGAR.
    const int random_seed;
    const double stagnation_limit;
    const double blacklist_trigger_percentage;
    const bool blacklist_on_stagnation;
    const double total_max_time;

    // Variables used in the main loop.
    bool blacklisting;
    double stagnation_start_time;
    int remaining_collection_size;

    void check_blacklist_trigger_timer(
        double blacklisting_start_time, const utils::CountdownTimer &timer);
    std::vector<bool> get_blacklisted_variables(
        std::vector<int> &non_goal_variables, int num_variables);
    void handle_generated_pattern(
        PatternCollectionInformation &&collection_info,
        utils::HashSet<Pattern> &generated_patterns,
        std::shared_ptr<PDBCollection> &generated_pdbs,
        const utils::CountdownTimer &timer);
    bool collection_size_limit_reached() const;
    bool time_limit_reached(const utils::CountdownTimer &timer) const;
    bool check_for_stagnation(const utils::CountdownTimer &timer);
public:
    explicit PatternCollectionGeneratorMultipleCegar(options::Options &opts);
    virtual ~PatternCollectionGeneratorMultipleCegar() = default;

    virtual PatternCollectionInformation generate(
        const std::shared_ptr<AbstractTask> &task) override;
};
}

#endif
