#ifndef SHRINK_STRATEGY_H
#define SHRINK_STRATEGY_H

#include <string>
#include <vector>
#include <ext/slist>

class Abstraction;

typedef int AbstractStateRef;

class ShrinkStrategy {
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

    // TODO: Get rid of this.
    enum {
        QUITE_A_LOT = 1000000000
    };

    bool must_shrink(const Abstraction &abs, int threshold, bool force) const;
    void apply(Abstraction &abs,
               EquivalenceRelation &equivalence_relation,
               int threshold) const;
public:
    ShrinkStrategy();
    virtual ~ShrinkStrategy();

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
    virtual void shrink(Abstraction &abs, int threshold,
                        bool force = false) const = 0;

    /* TODO: Would be nice not to need this three methods. This would
       require moving some of the responsibilities around a bit. */

    virtual bool has_memory_limit() const = 0;
    virtual bool is_bisimulation() const = 0;
    virtual bool is_dfp() const = 0;

    virtual std::string description() const = 0;
};

#endif
