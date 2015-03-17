#include "const_evaluator.h"

#include "option_parser.h"
#include "plugin.h"


ConstEvaluator::ConstEvaluator(const Options &opts)
    : Heuristic(opts),
      value(opts.get<int>("value")) {
}

int ConstEvaluator::compute_heuristic(const GlobalState &) {
    return value;
}

bool ConstEvaluator::is_dead_end() const {
    return false;
}

static Heuristic *_parse(OptionParser &parser) {
    parser.document_synopsis("Constant evaluator",
                             "Returns a constant value.");
    parser.add_option<int>("value", "the constant value", "1");
    Heuristic::add_options_to_parser(parser);
    Options opts = parser.parse();
    if (opts.get<int>("value") < 0) {
        parser.error("value must be >= 0");
    }
    if (parser.dry_run()) {
        return 0;
    } else {
        return new ConstEvaluator(opts);
    }
}

static Plugin<Heuristic> _plugin("const", _parse);
