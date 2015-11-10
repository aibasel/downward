// HACK! Ignore this if used as a top-level compile target.
#ifdef OPEN_LISTS_ALTERNATION_OPEN_LIST_H

#include "open_list_factory.h"

#include "../evaluation_context.h"
#include "../option_parser.h"

#include <cassert>
#include <cstdlib>
using namespace std;


template<class Entry>
AlternationOpenList<Entry>::AlternationOpenList(const Options &opts)
    : boost_amount(opts.get<int>("boost")) {
    vector<shared_ptr<OpenListFactory>> open_list_factories(
        opts.get_list<shared_ptr<OpenListFactory>>("sublists"));
    open_lists.reserve(open_list_factories.size());
    for (const auto &factory : open_list_factories)
        open_lists.push_back(factory->create_open_list<Entry>());

    priorities.resize(open_lists.size(), 0);
}

template<class Entry>
void AlternationOpenList<Entry>::do_insertion(
    EvaluationContext &eval_context, const Entry &entry) {
    for (const auto &sublist : open_lists)
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
    const auto &best_list = open_lists[best];
    assert(!best_list->empty());
    ++priorities[best];
    return best_list->remove_min(nullptr);
}

template<class Entry>
bool AlternationOpenList<Entry>::empty() const {
    for (const auto &sublist : open_lists)
        if (!sublist->empty())
            return false;
    return true;
}

template<class Entry>
void AlternationOpenList<Entry>::clear() {
    for (const auto &sublist : open_lists)
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
    for (const auto &sublist : open_lists)
        sublist->get_involved_heuristics(hset);
}

template<class Entry>
bool AlternationOpenList<Entry>::is_dead_end(
    EvaluationContext &eval_context) const {
    // If one sublist is sure we have a dead end, return true.
    if (is_reliable_dead_end(eval_context))
        return true;
    // Otherwise, return true if all sublists agree this is a dead-end.
    for (const auto &sublist : open_lists)
        if (!sublist->is_dead_end(eval_context))
            return false;
    return true;
}

template<class Entry>
bool AlternationOpenList<Entry>::is_reliable_dead_end(
    EvaluationContext &eval_context) const {
    for (const auto &sublist : open_lists)
        if (sublist->is_reliable_dead_end(eval_context))
            return true;
    return false;
}

#else

// HACK: We're the top level target.

#include "alternation_open_list.h"

#include "open_list_factory.h"

#include "../plugin.h"
#include "../utilities.h"

#include <memory>

using namespace std;


AlternationOpenListFactory::AlternationOpenListFactory(const Options &options)
    : options(options) {
}

unique_ptr<StateOpenList>
AlternationOpenListFactory::create_state_open_list() {
    return make_unique_ptr<AlternationOpenList<StateOpenListEntry>>(options);
}

unique_ptr<EdgeOpenList>
AlternationOpenListFactory::create_edge_open_list() {
    return make_unique_ptr<AlternationOpenList<EdgeOpenListEntry>>(options);
}

static shared_ptr<OpenListFactory> _parse(OptionParser &parser) {
    parser.document_synopsis("Alternation open list",
                             "alternates between several open lists.");
    parser.add_list_option<shared_ptr<OpenListFactory>>(
        "sublists",
        "open lists between which this one alternates");
    parser.add_option<int>(
        "boost",
        "boost value for contained open lists that are restricted "
        "to preferred successors",
        "0");

    Options opts = parser.parse();
    opts.verify_list_non_empty<shared_ptr<OpenListFactory>>("sublists");
    if (parser.dry_run())
        return nullptr;
    else
        return make_shared<AlternationOpenListFactory>(opts);
}

static PluginShared<OpenListFactory> _plugin("alt", _parse);

#endif
