#include "plugin.h"

#include "option_parser.h"

#include <string>

using namespace std;

ScalarEvaluatorPlugin::ScalarEvaluatorPlugin(
    const std::string &key, OptionParser::ScalarEvalFactory factory) {
    OptionParser::instance()->register_scalar_evaluator(key, factory);
}

ScalarEvaluatorPlugin::~ScalarEvaluatorPlugin() {
}
