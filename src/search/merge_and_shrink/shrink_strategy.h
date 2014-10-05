#ifndef MERGE_AND_SHRINK_SHRINK_STRATEGY_H
#define MERGE_AND_SHRINK_SHRINK_STRATEGY_H

#include <string>
#include <vector>
#include <ext/slist>

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
    typedef __gnu_cxx::slist<AbstractStateRef> EquivalenceClass;
    typedef std::vector<EquivalenceClass> EquivalenceRelation;
public:
    // HACK/TODO: The following method would usually be protected, but
    // the option parser requires it to be public for the
    // DefaultValueNamer. We need to reconsider the use of the
    // DefaultValueNamer here anyway, along with the way the default
    // strategy is specified. Maybe add a capability to have the default
    // value be a factory function with a string description?
    virtual std::string name() const = 0;
protected:
    virtual void dump_strategy_specific_options() const;

    std::pair<std::size_t, std::size_t> compute_shrink_sizes(
        std::size_t size1, std::size_t size2) const;
    bool must_shrink(const TransitionSystem &ts, int threshold) const;
    void apply(TransitionSystem &ts,
               EquivalenceRelation &equivalence_relation,
               int threshold) const;
public:
    ShrinkStrategy(const Options &opts);
    virtual ~ShrinkStrategy();

    /* Set this to true to apply label reduction before shrinking, in
       addition to the times when it is usually applied. Some shrink
       strategies may require labels to be reduced (e.g. ones based on
       bisimulations), and others might prefer reduced labels because
       it makes their computation more efficient.

       Set this to false if the shrink strategy does not benefit (or
       not much) from label reduction. Label reduction costs quite a
       bit of time and should not be performed without need.
    */
    virtual bool reduce_labels_before_shrinking() const = 0;

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
