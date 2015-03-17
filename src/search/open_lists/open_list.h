#ifndef OPEN_LISTS_OPEN_LIST_H
#define OPEN_LISTS_OPEN_LIST_H

#include <set>
#include <vector>

class EvaluationContext;


template<class Entry>
class OpenList {
    bool only_preferred;

protected:
    /*
      Insert an entry into the open list. This is called by insert, so
      see comments there. This method will not be called if
      is_reliable_dead_end() is true or if only_preferred is true and
      the entry to be inserted is not preferred. Hence, these
      conditions need not be checked by the implementation.
    */
    virtual void do_insertion(EvaluationContext &eval_context,
                              const Entry &entry) = 0;

public:
    explicit OpenList(bool preferred_only = false);
    virtual ~OpenList() = default;

    /*
      Insert an entry into the open list.

      This method may be called with entries that the open list does
      not want to insert (e.g. because they have an infinite estimate
      or because it is a non-preferred successor and the open list
      only wants preferred successors. In this case, the open list
      will remain unchanged.

      This method will often compute heuristic estimates etc. as a
      side effect, which is handled within the EvaluationContext
      class.

      Implementation note: uses the template method pattern, with
      do_insertion performing the bulk of the work. See comments for
      do_insertion.
    */
    void insert(EvaluationContext &eval_context, const Entry &entry);

    /*
      Remove and return the entry that should be expanded next.

      TODO: We want to eventually get rid of the "key" argument, since
      it breaks aspects of the abstraction. For example, see msg639 in
      the tracker. Currently, if key is non-null, it must point to an
      empty vector. Then remove_min stores the key for the popped
      element there.
    */
    virtual Entry remove_min(std::vector<int> *key = 0) = 0;

    // Return true if the open list is empty.
    virtual bool empty() const = 0;

    /*
      Remove all elements from the open list.

      TODO: This method might eventually go away, instead using a
      mechanism for generating new open lists at a higher level.
      This is currently only used be enforced hill-climbing.
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
      Add all heuristics that this open lists uses (directly or
      indirectly) into the result set.

      TODO: This method can probably go away at some point.
    */
    virtual void get_involved_heuristics(std::set<Heuristic *> &hset) = 0;

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

    bool only_contains_preferred_entries() const {
        return only_preferred;
    }

    /*
      Return true if the open list can guarantee that the state
      associated with the evaluation context is unsolvable.

      Usually, this happens when the open list uses a safe heuristic
      that assigns an infinite estimate to the state. It is always
      permissible for implementations of this method to return false,
      but this may lead to some extra search effort.

      Like OpenList::insert, this method will usually evaluate
      heuristic values, which are then stored in eval_context as a
      side effect.

      Intended usage: This should only be called "internally" (by
      OpenList::insert or by the is_reliable_dead_end implementations
      of open lists containing other open lists). OpenList::insert
      automatically calls this method, so there is no point in
      protecting calls to OpenList::insert with calls to this method.

      The main use case for this method is to implement "look before
      you leap" when inserting into open lists with multiple
      components, such as alternation open lists. If a given open list
      has several components and the second one reliable recognizes a
      state as a dead end, then the state should not be inserted into
      the first one either.
    */
    virtual bool is_reliable_dead_end(
        EvaluationContext &eval_context, const Entry &entry) = 0;
};

#include "open_list.cc"

// HACK! Need a better strategy of dealing with templates, also in the Makefile.

#endif
