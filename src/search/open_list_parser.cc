#include "open_lists/standard_scalar_open_list.h"
#include "open_lists/open_list_buckets.h"
#include "open_lists/tiebreaking_open_list.h"
#include "open_lists/alternation_open_list.h"
#include "open_lists/pareto_open_list.h"

#include <map>

template <class Entry>
OpenListParser<Entry> *OpenListParser<Entry>::instance_ = 0;

template <class Entry>
OpenListParser<Entry> *OpenListParser<Entry>::instance() {
    if (instance_ == 0) {
        instance_ = new OpenListParser<Entry>();

        // Register open lists
        OpenListParser<Entry>::instance()->register_open_list(
            "single", StandardScalarOpenList<Entry>::create);
        OpenListParser<Entry>::instance()->register_open_list(
            "single_buckets", BucketOpenList<Entry>::create);
        OpenListParser<Entry>::instance()->register_open_list(
            "tiebreaking", TieBreakingOpenList<Entry>::create);
        OpenListParser<Entry>::instance()->register_open_list(
            "alt", AlternationOpenList<Entry>::create);
        OpenListParser<Entry>::instance()->register_open_list(
            "pareto", ParetoOpenList<Entry>::create);
    }
    return instance_;
}

template <class Entry>
void OpenListParser<Entry>::register_open_list(
    const string &key, OpenList<Entry> *func(const vector<string> &, int, int &, bool)) {
    open_list_map[key] = func;
}

template <class Entry>
bool OpenListParser<Entry>::knows_open_list(const string &name) {
    return open_list_map.find(name) != open_list_map.end();
}

template <class Entry>
OpenList<Entry> *
OpenListParser<Entry>::parse_open_list(const vector<string> &input,
                                       int start, int &end, bool dry_run) {
    typename std::map<std::string, OpenListFactory>::iterator it;
    it = open_list_map.find(input[start]);
    if (it == open_list_map.end())
        throw ParseError(start);
    return it->second(input, start, end, dry_run);
}
