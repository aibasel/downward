#ifndef SHRINK_STRATEGY_H
#define SHRINK_STRATEGY_H

#include <string>
#include <vector>
#include <ext/slist>

class Abstraction;
class OptionParser;
class Options;

typedef int AbstractStateRef;

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

    std::pair<int, int> compute_shrink_sizes(int size1, int size2) const;
    bool must_shrink(const Abstraction &abs, int threshold, bool force) const;
    void apply(Abstraction &abs,
               EquivalenceRelation &equivalence_relation,
               int threshold) const;
public:
    ShrinkStrategy(const Options &opts);
    virtual ~ShrinkStrategy();

    enum WhenToNormalize {BEFORE_MERGE, AFTER_MERGE};
    virtual WhenToNormalize when_to_normalize(
        bool use_label_reduction) const = 0;

    void dump_options() const;

    /* TODO: Make sure that *all* shrink strategies prune irrelevant
       and unreachable states, then update documentation below
       accordingly? Currently the only exception is ShrinkRandom, I
       think. */

    /* Shrink the abstraction to size threshold.

       In most shrink stategies, this also prunes all irrelevant and
       unreachable states, which may cause the resulting size to be
       lower than threshold.

       Does nothing if threshold >= abs.size() unless force is true
       (in which case it only prunes irrelevant and unreachable
       states).
    */

    // TODO: Should all three of these be virtual?
    // TODO: Should all three of these be public?
    //       If not, also modify in derived clases.

    virtual void shrink(Abstraction &abs, int threshold,
                        bool force = false) = 0;
    virtual void shrink_atomic(Abstraction &abs1);
    virtual void shrink_before_merge(Abstraction &abs1, Abstraction &abs2);

    static void add_options_to_parser(OptionParser &parser);
    static void handle_option_defaults(Options &opts);
};

#endif
