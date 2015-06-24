#ifndef MERGE_AND_SHRINK_SHRINK_STRATEGY_H
#define MERGE_AND_SHRINK_SHRINK_STRATEGY_H

#include <string>
#include <vector>
#include <forward_list>

class TransitionSystem;
class OptionParser;
class Options;

class ShrinkStrategy {
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
private:
    // The maximum size of a transition system at any point.
    const int max_states;
    // The maximum size of a transition system before being merged.
    const int max_states_before_merge;
    /*
      shrink_threshold_before_merge: Shrink the transition system iff it is
      larger than this size. Note that this is set independently from
      max_states, which is the number of states to which the transition
      system must be shrunk to respect the given limit.
    */
    const int shrink_threshold_before_merge;

    bool abstract_transition_system(TransitionSystem &ts, int new_size);
    std::pair<std::size_t, std::size_t> compute_shrink_sizes(
        std::size_t size1, std::size_t size2) const;
protected:
    /*
      Shrink the transition system to a size at most target. This can be
      called even if the current size respects the target size already
      (if shrink_threshold_before_merge is smaller).
    */
    virtual void shrink(const TransitionSystem &ts,
                        int target,
                        StateEquivalenceRelation &equivalence_relation) = 0;
    virtual std::string name() const = 0;
    virtual void dump_strategy_specific_options() const = 0;
public:
    ShrinkStrategy(const Options &opts);
    virtual ~ShrinkStrategy();

    std::pair<bool, bool> shrink_before_merge(TransitionSystem &ts1, TransitionSystem &ts2);

    void dump_options() const;
    std::string get_name() const;

    static void add_options_to_parser(OptionParser &parser);
    static void handle_option_defaults(Options &opts);
};

#endif
