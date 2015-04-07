#ifndef MERGE_AND_SHRINK_SHRINK_STRATEGY_H
#define MERGE_AND_SHRINK_SHRINK_STRATEGY_H

#include <string>
#include <vector>
#include <forward_list>

class TransitionSystem;
class OptionParser;
class Options;

class ShrinkStrategy {
    const int max_states;
    const int max_states_before_merge;
protected:
    /* An equivalence class is a set of abstract states that shall be
       mapped (shrunk) to the same abstract state.

       An equivalence relation is a partitioning of states into
       equivalence classes. It may omit certain states entirely; these
       will be dropped completely and receive an h value of infinity.
       This is used to remove unreachable and irrelevant states.
    */

    typedef int AbstractStateRef;
    typedef std::forward_list<AbstractStateRef> StateEquivalenceClass;
    typedef std::vector<StateEquivalenceClass> StateEquivalenceRelation;

    virtual std::string name() const = 0;
    virtual void dump_strategy_specific_options() const;

    std::pair<std::size_t, std::size_t> compute_shrink_sizes(
        std::size_t size1, std::size_t size2) const;
    bool must_shrink(const TransitionSystem &ts, int threshold) const;
    void apply(TransitionSystem &ts,
               StateEquivalenceRelation &equivalence_relation,
               int threshold) const;
public:
    ShrinkStrategy(const Options &opts);
    virtual ~ShrinkStrategy();

    void dump_options() const;

    /* TODO: Make sure that *all* shrink strategies prune irrelevant
       and unreachable states, then update documentation below
       accordingly? Currently the only exception is ShrinkRandom, I
       think. */

    /* Shrink the transition system to size threshold.

       In most shrink stategies, this also prunes all irrelevant and
       unreachable states, which may cause the resulting size to be
       lower than threshold.

       Does nothing if threshold >= ts.size() unless force is true
       (in which case it only prunes irrelevant and unreachable
       states).
    */

    // TODO: Should all three of these be virtual?
    // TODO: Should all three of these be public?
    //       If not, also modify in derived clases.

    virtual void shrink(TransitionSystem &ts, int threshold) = 0;
    virtual void shrink_before_merge(TransitionSystem &ts1, TransitionSystem &ts2);

    static void add_options_to_parser(OptionParser &parser);
    static void handle_option_defaults(Options &opts);
};

#endif
