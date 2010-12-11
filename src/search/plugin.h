#ifndef PLUGIN_H
#define PLUGIN_H

#include "option_parser.h"


class ScalarEvaluatorPlugin {
    ScalarEvaluatorPlugin(const ScalarEvaluatorPlugin &copy);
public:
    ScalarEvaluatorPlugin(const std::string &key,
                          OptionParser::ScalarEvalFactory factory);
    ~ScalarEvaluatorPlugin();
};


class SynergyPlugin {
    SynergyPlugin(const SynergyPlugin &copy);
public:
    SynergyPlugin(const std::string &key,
                  OptionParser::SynergyFactory factory);
    ~SynergyPlugin();
};


class LandmarkGraphPlugin {
    LandmarkGraphPlugin(const LandmarkGraphPlugin &copy);
public:
    LandmarkGraphPlugin(const std::string &key,
                        OptionParser::LandmarkGraphFactory factory);
    ~LandmarkGraphPlugin();
};

#endif
