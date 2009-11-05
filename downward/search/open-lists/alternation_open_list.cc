// HACK! Ignore this if used as a top-level compile target.
#ifdef ALTERNATION_OPENLIST_H

#include <iostream>
#include <cassert>
using namespace std;

template<class Entry>
AlternationOpenList<Entry>::AlternationOpenList(const vector<OpenList<Entry> *> &sublists) 
    : open_lists(sublists), priorities(sublists.size(), 0), size(0) {
}

template<class Entry>
AlternationOpenList<Entry>::~AlternationOpenList() {
}

template<class Entry>
int AlternationOpenList<Entry>::insert(const Entry &entry) {
	int new_entries = 0;
    for (unsigned int i = 0; i < open_lists.size(); i++) {
		new_entries += open_lists[i]->insert(entry);
    }
    size += new_entries;
	return new_entries;
}

template<class Entry>
Entry AlternationOpenList<Entry>::remove_min() {
    assert(size > 0);
    int best = 0;
    for (unsigned int i = 0; i < open_lists.size(); i++) {
        if (!open_lists[i]->empty() &&
            priorities[i] < priorities[best]) {
            best = i;
        }
    }
    OpenList<Entry>* best_list = open_lists[best];
    assert (!best_list->empty());
    size--;
    priorities[best]++;
    return best_list->remove_min();
}

template<class Entry>
bool AlternationOpenList<Entry>::empty() const {
    return size == 0;
}

template<class Entry>
void AlternationOpenList<Entry>::evaluate(int g, bool preferred) {
    dead_end = false;
    dead_end_reliable = false;
    for (unsigned int i = 0; i < open_lists.size(); i++) {
        open_lists[i]->evaluate(g, preferred);
        if (open_lists[i]->is_dead_end()) {
            dead_end = true;
            if (open_lists[i]->dead_end_is_reliable()) {
                dead_end_reliable = true;
                break;
            }
        }
	}
}

template<class Entry>
bool AlternationOpenList<Entry>::is_dead_end() const {
    return dead_end;
}

template<class Entry>
bool AlternationOpenList<Entry>::dead_end_is_reliable() const {
    return dead_end_reliable;
}

#endif
