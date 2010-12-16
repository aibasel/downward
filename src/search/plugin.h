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

class SearchEngine;
class LandmarksGraph;
class ScalarEvaluator;
class OptionParser;
class Heuristic;
template <class Entry>
class OpenList;


class ScalarEvaluatorPlugin {
      ScalarEvaluatorPlugin(const ScalarEvaluatorPlugin &copy);
public:
    ScalarEvaluatorPlugin(const std::string &key,
                          Registry<ScalarEvaluator *>::Factory factory);
    ~ScalarEvaluatorPlugin();
};


class SynergyPlugin {
    SynergyPlugin(const SynergyPlugin &copy);
public:
    //there isn't really a Synergy class, it's just a pseudoclass 
    //to unify syntax.
    SynergyPlugin(const std::string &key,
                  Registry<Synergy *>::Factory factory);
    ~SynergyPlugin();
};


class LandmarkGraphPlugin {
    LandmarkGraphPlugin(const LandmarkGraphPlugin &copy);
public:
    LandmarkGraphPlugin(const std::string &key,
                        Registry<LandmarksGraph *>::Factory factory);
    ~LandmarkGraphPlugin();
};


class EnginePlugin {
    EnginePlugin(const EnginePlugin &copy);
public:
    EnginePlugin(const std::string &key,
                 Registry<SearchEngine *>::Factory factory);
    ~EnginePlugin();
};

template <class Entry>
class OpenListPlugin {
    OpenListPlugin(const OpenListPlugin<Entry> &copy);
public:
    OpenListPlugin(const std::string &key,
                   typename Registry<OpenList<Entry > *>::Factory factory) {
        std::cout << "registering openlist " << key << std::endl;
        Registry<OpenList<Entry > *>::instance()->register_object(key, factory);
    }
    ~OpenListPlugin();

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
