#include "plugin.h"

#include "option_parser.h"

#include <string>

using namespace std;


ScalarEvaluatorPlugin::ScalarEvaluatorPlugin(
    const string &key, OptionParser::ScalarEvalFactory factory) {
    OptionParser::instance()->register_scalar_evaluator(key, factory);
}

ScalarEvaluatorPlugin::~ScalarEvaluatorPlugin() {
}

SynergyPlugin::SynergyPlugin(
    const string &key, OptionParser::SynergyFactory factory) {
    OptionParser::instance()->register_synergy(key, factory);
}

SynergyPlugin::~SynergyPlugin() {
}

LandmarkGraphPlugin::LandmarkGraphPlugin(
    const string &key, OptionParser::LandmarkGraphFactory factory) {
    OptionParser::instance()->register_lm_graph_factory(key, factory);
}

LandmarkGraphPlugin::~LandmarkGraphPlugin() {
}
