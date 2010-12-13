#ifndef PLUGIN_H
#define PLUGIN_H

#include "option_parser.h"
#include "search_engine.h"

class LandmarksGraph;

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


#endif
