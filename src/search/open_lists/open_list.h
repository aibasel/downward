#ifndef OPEN_LISTS_OPEN_LIST_H
#define OPEN_LISTS_OPEN_LIST_H

#include "../evaluator.h"
#include <vector>

template<class Entry>
class OpenList : public Evaluator {
protected:
    virtual Evaluator *get_evaluator() = 0;
    bool only_preferred;

public:
    OpenList(bool preferred_only = false) : only_preferred(preferred_only) {}
    virtual ~OpenList() {}

    virtual int insert(const Entry &entry) = 0;
    virtual Entry remove_min(std::vector<int> *key = 0) = 0;
    // If key is non-null, it must point to an empty vector.
    // Then remove_min stores the key for the popped element there.
    // TODO: We might want to solve this differently eventually;
    //       see msg639 in the tracker.
    virtual bool empty() const = 0;
    virtual void clear() = 0;
    bool only_preferred_states() const {return only_preferred; }
    // should only be used within alternation open lists
    // a search does not have to care about this because
    // it is handled by the open list whether the entry will
    // be inserted

    virtual int boost_preferred() {return 0; }
    virtual void boost_last_used_list() {return; }
};

#endif
