#ifndef MERGE_AND_SHRINK_MERGE_TREE_FACTORY_H
#define MERGE_AND_SHRINK_MERGE_TREE_FACTORY_H

#include <memory>

class AbstractTask;

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

class MergeTreeFactory {
protected:
    std::shared_ptr<utils::RandomNumberGenerator> rng;
    virtual std::string name() const = 0;
    virtual void dump_tree_specific_options() const {}
public:
    explicit MergeTreeFactory(const options::Options &options);
    virtual ~MergeTreeFactory() = default;
    void dump_options() const;
    virtual std::unique_ptr<MergeTree> compute_merge_tree(
        std::shared_ptr<AbstractTask> task,
        FactoredTransitionSystem &fts) = 0;
    // Inheriting classes must call this method in their parsing methods.
    static void add_options_to_parser(options::OptionParser &parser);
};
}

#endif
