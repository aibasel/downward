// HACK! Ignore this if used as a top-level compile target.
#ifdef OPEN_LISTS_ALTERNATION_OPEN_LIST_H

#include "../evaluation_context.h"
#include "../option_parser.h"

#include <cassert>
#include <cstdlib>
using namespace std;


template<class Entry>
AlternationOpenList<Entry>::AlternationOpenList(const Options &opts)
    : open_lists(opts.get_list<OpenList<Entry> *>("sublists")),
      priorities(open_lists.size(), 0),
      boost_amount(opts.get<int>("boost")) {
}

template<class Entry>
AlternationOpenList<Entry>::AlternationOpenList(
    const vector<OpenList<Entry> *> &sublists,
    int boost_amount)
    : open_lists(sublists),
      priorities(sublists.size(), 0),
      boost_amount(boost_amount) {
}

template<class Entry>
void AlternationOpenList<Entry>::do_insertion(
    EvaluationContext &eval_context, const Entry &entry) {
    for (OpenList<Entry> *sublist : open_lists)
        sublist->insert(eval_context, entry);
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
    OpenList<Entry> *best_list = open_lists[best];
    assert(!best_list->empty());
    ++priorities[best];
    return best_list->remove_min(nullptr);
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
    for (OpenList<Entry> *sublist : open_lists)
        sublist->clear();
}

template<class Entry>
void AlternationOpenList<Entry>::boost_preferred() {
    for (size_t i = 0; i < open_lists.size(); ++i)
        if (open_lists[i]->only_contains_preferred_entries())
            priorities[i] -= boost_amount;
}

template<class Entry>
void AlternationOpenList<Entry>::get_involved_heuristics(
    set<Heuristic *> &hset) {
    for (OpenList<Entry> *sublist : open_lists)
        sublist->get_involved_heuristics(hset);
}

template<class Entry>
bool AlternationOpenList<Entry>::is_dead_end(
    EvaluationContext &eval_context) const {
    // If one sublist is sure we have a dead end, return true.
    if (is_reliable_dead_end(eval_context))
        return true;
    // Otherwise, return true if all sublists agree this is a dead-end.
    for (OpenList<Entry> *sublist : open_lists)
        if (!sublist->is_dead_end(eval_context))
            return false;
    return true;
}

template<class Entry>
bool AlternationOpenList<Entry>::is_reliable_dead_end(
    EvaluationContext &eval_context) const {
    for (OpenList<Entry> *sublist : open_lists)
        if (sublist->is_reliable_dead_end(eval_context))
            return true;
    return false;
}

template<class Entry>
OpenList<Entry> *AlternationOpenList<Entry>::_parse(OptionParser &parser) {
    parser.document_synopsis("Alternation open list",
                             "alternates between several open lists.");
    parser.add_list_option<OpenList<Entry> *>(
        "sublists",
        "open lists between which this one alternates");
    parser.add_option<int>(
        "boost",
        "boost value for contained open lists that are restricted "
        "to preferred successors",
        "0");

    Options opts = parser.parse();
    opts.verify_list_non_empty<OpenList<Entry> *>("sublists");
    if (parser.dry_run())
        return nullptr;
    else
        return new AlternationOpenList<Entry>(opts);
}

#endif
