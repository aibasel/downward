#include "plugin.h"

#include "option_parser.h"

#include <string>

using namespace std;


ScalarEvaluatorPlugin::ScalarEvaluatorPlugin(
    const string &key, Registry<ScalarEvaluator *>::Factory factory) {
    Registry<ScalarEvaluator *>::instance()->register_object(key, factory);
}

ScalarEvaluatorPlugin::~ScalarEvaluatorPlugin() {
}

SynergyPlugin::SynergyPlugin(
    const string &key, Registry<Synergy *>::Factory factory) {
    Registry<Synergy *>::instance()->register_object(key, factory);
}

SynergyPlugin::~SynergyPlugin() {
}

LandmarkGraphPlugin::LandmarkGraphPlugin(
    const string &key, Registry<LandmarksGraph *>::Factory factory) {
    Registry<LandmarksGraph *>::instance()->register_object(key, factory);
}

LandmarkGraphPlugin::~LandmarkGraphPlugin() {
}

EnginePlugin::EnginePlugin(
    const string &key, Registry<SearchEngine *>::Factory factory) {
    Registry<SearchEngine *>::instance()->register_object(key, factory);
}

EnginePlugin::~EnginePlugin() {
}
