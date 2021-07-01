#ifndef PDBS_PATTERN_COLLECTION_GENERATOR_MULTIPLE_H
#define PDBS_PATTERN_COLLECTION_GENERATOR_MULTIPLE_H

#include "pattern_generator.h"

#include <set>
#include <unordered_set>

namespace options {
class OptionParser;
class Options;
}

namespace utils {
class CountdownTimer;
class RandomNumberGenerator;
enum class Verbosity;
}

namespace pdbs {
class PatternCollectionGeneratorMultiple : public PatternCollectionGenerator {
    const std::string name;
    const int max_pdb_size;
    const double pattern_generation_max_time;
    const double total_max_time;
    const double stagnation_limit;
    const double blacklisting_start_time;
    const bool enable_blacklist_on_stagnation;
    const utils::Verbosity verbosity;
    std::shared_ptr<utils::RandomNumberGenerator> rng;
    const int random_seed;

    // Variables used in the main loop.
    int remaining_collection_size;
    bool blacklisting;
    double time_point_of_last_new_pattern;

    void check_blacklist_trigger_timer(const utils::CountdownTimer &timer);
    std::unordered_set<int> get_blacklisted_variables(
        std::vector<int> &non_goal_variables);
    void handle_generated_pattern(
        PatternInformation &&pattern_info,
        std::set<Pattern> &generated_patterns,
        std::shared_ptr<PDBCollection> &generated_pdbs,
        const utils::CountdownTimer &timer);
    bool collection_size_limit_reached() const;
    bool time_limit_reached(const utils::CountdownTimer &timer) const;
    bool check_for_stagnation(const utils::CountdownTimer &timer);
protected:
    virtual void initialize(const std::shared_ptr<AbstractTask> &task) = 0;
    virtual PatternInformation compute_pattern(
        int max_pdb_size,
        double max_time,
        const std::shared_ptr<utils::RandomNumberGenerator> &rng,
        const std::shared_ptr<AbstractTask> &task,
        const FactPair &goal,
        std::unordered_set<int> &&blacklisted_variables) = 0;
public:
    PatternCollectionGeneratorMultiple(options::Options &opts, const std::string &name);
    virtual ~PatternCollectionGeneratorMultiple() override = default;
    virtual PatternCollectionInformation generate(
        const std::shared_ptr<AbstractTask> &task) override;
};

extern void add_multiple_algorithm_implementation_notes_to_parser(
    options::OptionParser &parser);
extern void add_multiple_options_to_parser(options::OptionParser &parser);
}

#endif
