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
    const double stagnation_limit;
    const double blacklist_trigger_percentage;
    const bool enable_blacklist_on_stagnation;
    const double total_max_time;

    // Variables used in the main loop.
    int remaining_collection_size;
    bool blacklisting;
    double stagnation_start_time;

    void check_blacklist_trigger_timer(
        double blacklisting_start_time, const utils::CountdownTimer &timer);
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
    const utils::Verbosity verbosity;
    std::shared_ptr<utils::RandomNumberGenerator> rng;

    virtual std::string get_name() const = 0;
    virtual void initialize(const std::shared_ptr<AbstractTask> &task) = 0;
    virtual PatternInformation compute_pattern(
        const std::shared_ptr<AbstractTask> &task,
        FactPair goal,
        std::unordered_set<int> &&blacklisted_variables,
        const utils::CountdownTimer &timer,
        int remaining_collection_size) = 0;
public:
    explicit PatternCollectionGeneratorMultiple(options::Options &opts);
    virtual PatternCollectionInformation generate(
        const std::shared_ptr<AbstractTask> &task) override;
};

extern void add_multiple_options_to_parser(options::OptionParser &parser);
}

#endif
