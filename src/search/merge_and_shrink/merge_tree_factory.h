#ifndef MERGE_AND_SHRINK_MERGE_TREE_FACTORY_H
#define MERGE_AND_SHRINK_MERGE_TREE_FACTORY_H

#include <memory>
#include <vector>

class TaskProxy;

namespace options {
class OptionParser;
class Options;
}

namespace utils {
class RandomNumberGenerator;
}

namespace merge_and_shrink {
class FactoredTransitionSystem;
class MergeTree;
enum class UpdateOption;

class MergeTreeFactory {
protected:
    std::shared_ptr<utils::RandomNumberGenerator> rng;
    UpdateOption update_option;
    virtual std::string name() const = 0;
    virtual void dump_tree_specific_options() const {}
public:
    explicit MergeTreeFactory(const options::Options &options);
    virtual ~MergeTreeFactory() = default;
    void dump_options() const;
    // Compute a merge tree for the given entire task
    virtual std::unique_ptr<MergeTree> compute_merge_tree(
        const TaskProxy &task_proxy) = 0;
    /* Compute a merge tree for the given current factored transition,
       system, possibly for a sbuset of indices. */
    virtual std::unique_ptr<MergeTree> compute_merge_tree(
        const TaskProxy &task_proxy,
        const FactoredTransitionSystem &fts,
        const std::vector<int> &indices_subset);
    // Inheriting classes must call this method in their parsing methods.
    static void add_options_to_parser(options::OptionParser &parser);
};
}

#endif
