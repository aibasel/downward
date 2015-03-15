#ifndef OPEN_LISTS_OPEN_LIST_H
#define OPEN_LISTS_OPEN_LIST_H

#include <vector>

class EvaluationContext;


template<class Entry>
class OpenList {
protected:
    // TODO: Get rid of this? If we want it, perhaps we don't want it
    // in the base class.
    bool only_preferred;

    /*
      Return true if the open list can guarantee that the state
      associated with the evaluation context is unsolvable.

      Usually, this happens when the open list uses a safe heuristic
      that assigns an infinite estimate to the state. It is always
      permissible to return false, but this may lead to some extra
      search effort.

      Like OpenList::insert, this method will usually evaluate
      heuristic values, which are then stored in eval_context as a
      side effect.

      Intended usage: in some ways, this method is a "dry-run" version
      of OpenList::insert, as it has to evaluate the state in the same
      way as insert does in order to determine if it is a dead end.
      Callers should not call is_reliable_dead_end to avoid calling
      insert on dead-end states, as insert is expected to contain the
      necessary logic itself. This is why the method is protected.

      The main use case for this method is to implement "look before
      you leap" in the insert method of open lists with multiple
      components, such as alternation open lists. If a given open list
      has several components and the second one reliable recognizes a
      state as a dead end, then the state should not be inserted into
      the first one either.

      TODO: For now (while we don't have implementations for the
      derived classes), the default implementation returns false, but
      it would probably be better to make this pure virtual.
    */
    virtual bool is_reliable_dead_end(
        EvaluationContext &eval_context, const Entry &entry);

public:
    // TODO: Not sure if this preferred_only argument belongs here and
    //       should stay.
    explicit OpenList(bool preferred_only = false);
    virtual ~OpenList() = default;

    /*
      Insert an entry into the open list.

      This method may be called with entries that the open list does
      not want to insert (e.g. because they have an infinite estimate
      or because it is a non-preferred successor and the open list
      only wants preferred successors. In this case, the open list
      should simply not insert the entry.

      This method will often compute heuristic estimates etc. as a
      side effect, which is handled within the EvaluationContext
      class.
    */
    virtual void insert(EvaluationContext &eval_context,
                        const Entry &entry) = 0;

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
      TODO: The following method needs cleaning up. Do we need it at
      all? Old comment follows:

      should only be used within alternation open lists a search does
      not have to care about this because it is handled by the open
      list whether the entry will be inserted
    */
    bool only_preferred_states() const {
        return only_preferred;
    }
};

#include "open_list.cc"

// HACK! Need a better strategy of dealing with templates, also in the Makefile.

#endif
