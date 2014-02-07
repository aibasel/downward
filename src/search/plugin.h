#ifndef PLUGIN_H
#define PLUGIN_H

#include <vector>
#include <string>
#include <map>
#include <iostream>
#include "open_lists/standard_scalar_open_list.h"
#include "open_lists/open_list_buckets.h"
#include "open_lists/tiebreaking_open_list.h"
#include "open_lists/alternation_open_list.h"
#include "open_lists/pareto_open_list.h"

template <class T>
class Plugin {
    Plugin(const Plugin<T> &copy);
public:
    Plugin(const std::string &key, typename Registry<T *>::Factory factory) {
        Registry<T *>::
        instance()->register_object(key, factory);
    }
    ~Plugin() {}
};

template <class Entry>
class Plugin<OpenList<Entry > > {
    Plugin(const Plugin<OpenList<Entry > > &copy);
public:
    ~Plugin();

    static void register_open_lists() {
        Registry<OpenList<Entry > *>::instance()->register_object(
            "single", StandardScalarOpenList<Entry>::_parse);
        Registry<OpenList<Entry > *>::instance()->register_object(
            "single_buckets", BucketOpenList<Entry>::_parse);
        Registry<OpenList<Entry > *>::instance()->register_object(
            "tiebreaking", TieBreakingOpenList<Entry>::_parse);
        Registry<OpenList<Entry > *>::instance()->register_object(
            "alt", AlternationOpenList<Entry>::_parse);
        Registry<OpenList<Entry > *>::instance()->register_object(
            "pareto", ParetoOpenList<Entry>::_parse);
    }
};

#endif
