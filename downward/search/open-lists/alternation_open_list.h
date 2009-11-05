#ifndef ALTERNATION_OPENLIST_H
#define ALTERNATION_OPENLIST_H

#include "open_list.h"
#include "../evaluator.h"

#include <vector>

template<class Entry>
class AlternationOpenList : public OpenList<Entry>, public Evaluator {
    std::vector<OpenList<Entry> *> open_lists; 
    std::vector<int> priorities;

    int size;
	bool dead_end;
    bool dead_end_reliable;

protected:
    Evaluator* get_evaluator() { return this; }

public:
    AlternationOpenList(const vector<OpenList<Entry> *> &sublists);
    ~AlternationOpenList();

    // OpenList interface
    int insert(const Entry &entry);
    Entry remove_min();
    bool empty() const;

    // Evaluator interface
    void evaluate(int g, bool preferred);
    bool is_dead_end() const;
    bool dead_end_is_reliable() const;
};

#include "alternation_open_list.cc"

// HACK! Need a better strategy of dealing with templates, also in the Makefile.

#endif
