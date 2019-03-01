#ifndef OPEN_LIST_H
#define OPEN_LIST_H

#include <set>

#include "evaluation_context.h"
#include "operator_id.h"

class StateID;


template<class Entry>
class OpenList {
    bool only_preferred;

protected:
    /*
      Insert an entry into the open list. This is called by insert, so
      see comments there. This method will not be called if
      is_dead_end() is true or if only_preferred is true and the entry
      to be inserted is not preferred. Hence, these conditions need
      not be checked by the implementation.
    */
    virtual void do_insertion(EvaluationContext &eval_context,
                              const Entry &entry) = 0;

public:
    explicit OpenList(bool preferred_only = false);
    virtual ~OpenList() = default;

    /*
      Insert an entry into the open list.

      This method may be called with entries that the open list does
      not want to insert, e.g. because they have an infinite estimate
      or because they are non-preferred successor and the open list
      only wants preferred successors. In this case, the open list
      will remain unchanged.

      This method will often compute heuristic estimates as a side
      effect, which are cached in the EvaluationContext object that
      is passed in.

      Implementation note: uses the template method pattern, with
      do_insertion performing the bulk of the work. See comments for
      do_insertion.
    */
    void insert(EvaluationContext &eval_context, const Entry &entry);

    /*
      Remove and return the entry that should be expanded next.
    */
    virtual Entry remove_min() = 0;

    // Return true if the open list is empty.
    virtual bool empty() const = 0;

    /*
      Remove all elements from the open list.

      TODO: This method might eventually go away, instead using a
      mechanism for generating new open lists at a higher level.
      This is currently only used by enforced hill-climbing.
    */
    virtual void clear() = 0;

    /*
      Called when the search algorithm wants to "boost" open lists
      using preferred successors.

      The default implementation does nothing. The main use case for
      this is for alternation open lists.

      TODO: Might want to change the name of the method to something
      generic that reflects when it is called rather than how one
      particular open list handles the event. I think this is called
      on "search progress", so we should verify if this is indeed the
      case, clarify what exactly that means, and then rename the
      method to reflect this (e.g. with a name like
      "on_search_progress").
    */
    virtual void boost_preferred();

    /*
      Add all path-dependent evaluators that this open lists uses (directly or
      indirectly) into the result set.

      TODO: This method can probably go away at some point.
    */
    virtual void get_path_dependent_evaluators(
        std::set<Evaluator *> &evals) = 0;

    /*
      Accessor method for only_preferred.

      The only use case for this at the moment is for alternation open
      lists, which boost those sublists which only include preferred
      entries.

      TODO: Is this sufficient reason to have this method? We could
      get rid of it if instead AlternationOpenList would be passed
      information upon construction which lists should be boosted on
      progress. This could also make the code more general (it would
      be easy to boost different lists by different amounts, and
      boosting would not have to be tied to preferredness). We should
      discuss the best way to proceed here.
    */
    bool only_contains_preferred_entries() const;

    /*
      is_dead_end and is_reliable_dead_end return true if the state
      associated with the passed-in evaluation context is deemed a
      dead end by the open list.

      The difference between the two methods is that
      is_reliable_dead_end must guarantee that the associated state is
      actually unsolvable, i.e., it must not believe the claims of
      unsafe heuristics.

      Like OpenList::insert, the methods usually evaluate heuristic
      values, which are then cached in eval_context as a side effect.
    */
    virtual bool is_dead_end(EvaluationContext &eval_context) const = 0;
    virtual bool is_reliable_dead_end(
        EvaluationContext &eval_context) const = 0;
};


using StateOpenListEntry = StateID;
using EdgeOpenListEntry = std::pair<StateID, OperatorID>;

using StateOpenList = OpenList<StateOpenListEntry>;
using EdgeOpenList = OpenList<EdgeOpenListEntry>;


template<class Entry>
OpenList<Entry>::OpenList(bool only_preferred)
    : only_preferred(only_preferred) {
}

template<class Entry>
void OpenList<Entry>::boost_preferred() {
}

template<class Entry>
void OpenList<Entry>::insert(
    EvaluationContext &eval_context, const Entry &entry) {
    if (only_preferred && !eval_context.is_preferred())
        return;
    if (!is_dead_end(eval_context))
        do_insertion(eval_context, entry);
}

template<class Entry>
bool OpenList<Entry>::only_contains_preferred_entries() const {
    return only_preferred;
}

#endif
