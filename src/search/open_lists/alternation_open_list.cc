// HACK! Ignore this if used as a top-level compile target.
#ifdef OPEN_LISTS_ALTERNATION_OPEN_LIST_H

#include "../option_parser.h"

#include <cassert>
#include <cstdlib>
using namespace std;


template<class Entry>
OpenList<Entry> *AlternationOpenList<Entry>::_parse(OptionParser &parser) {
    parser.document_synopsis("Alternation open list",
                             "alternates between several open lists.");
    parser.add_list_option<OpenList<Entry> *>("sublists", "sub open lists");
    parser.add_option<int>("boost",
                           "boost value for sub-open-lists "
                           "that are restricted to preferred operator nodes",
                           "0");

    Options opts = parser.parse();
    if (parser.help_mode())
        return 0;

    if (opts.get_list<OpenList<Entry> *>("sublists").empty())
        parser.error("need at least one internal open list");
    if (parser.dry_run())
        return 0;
    else
        return new AlternationOpenList<Entry>(opts);
}

template<class Entry>
AlternationOpenList<Entry>::AlternationOpenList(const Options &opts)
    : open_lists(opts.get_list<OpenList<Entry> *>("sublists")),
      priorities(open_lists.size(), 0),
      boosting(opts.get<int>("boost")) {
}

template<class Entry>
AlternationOpenList<Entry>::AlternationOpenList(const vector<OpenList<Entry> *> &sublists,
                                                int boost_influence)
    : open_lists(sublists), priorities(sublists.size(), 0),
      boosting(boost_influence) {
}

template<class Entry>
AlternationOpenList<Entry>::~AlternationOpenList() {
}

template<class Entry>
void AlternationOpenList<Entry>::insert(
    EvaluationContext &eval_context, const Entry &entry) {
    for (size_t i = 0; i < open_lists.size(); ++i)
        open_lists[i]->insert(eval_context, entry);
}

template<class Entry>
Entry AlternationOpenList<Entry>::remove_min(vector<int> *key) {
    if (key) {
        cerr << "not implemented -- see msg639 in the tracker" << endl;
        exit_with(EXIT_UNSUPPORTED);
    }
    int best = -1;
    for (size_t i = 0; i < open_lists.size(); ++i) {
        if (!open_lists[i]->empty() &&
            (best == -1 || priorities[i] < priorities[best])) {
            best = i;
        }
    }
    assert(best != -1);
    last_used_list = best;
    OpenList<Entry> *best_list = open_lists[best];
    assert(!best_list->empty());
    ++priorities[best];
    return best_list->remove_min(0);
}

template<class Entry>
bool AlternationOpenList<Entry>::empty() const {
    for (const OpenList<Entry> *sublist : open_lists)
        if (!sublist->empty())
            return false;
    return true;
}

template<class Entry>
void AlternationOpenList<Entry>::clear() {
    for (size_t i = 0; i < open_lists.size(); ++i)
        open_lists[i]->clear();
}

template<class Entry>
void AlternationOpenList<Entry>::evaluate(int g, bool preferred) {
    for (OpenList<Entry> *sublist : open_lists)
        sublist->evaluate(g, preferred);
}

template<class Entry>
void AlternationOpenList<Entry>::get_involved_heuristics(std::set<Heuristic *> &hset) {
    for (size_t i = 0; i < open_lists.size(); ++i)
        open_lists[i]->get_involved_heuristics(hset);
}

template<class Entry>
int AlternationOpenList<Entry>::boost_preferred() {
    int total_boost = 0;
    for (size_t i = 0; i < open_lists.size(); ++i) {
        // if the open list is not an alternation open list
        // (these have always only_preferred==false) and
        // it takes only preferred states, we boost it
        if (open_lists[i]->only_preferred_states()) {
            priorities[i] -= boosting;
            total_boost += boosting;
        }
        // otherwise, we tell it to boost its lists (which
        // has no effect on non-alterntion lists)
        else {
            int boosted = open_lists[i]->boost_preferred();
            // now we have to boost this alternation open list
            // as well to give its boosting some effect
            priorities[i] -= boosted;
            total_boost += boosted;
        }
    }
    return total_boost; // can be used by "parent" alternation list
}

#endif
