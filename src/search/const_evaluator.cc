#include "const_evaluator.h"

#include "option_parser.h"
#include "plugin.h"


ConstEvaluator::ConstEvaluator(const Options &opts)
    : Heuristic(opts),
      value(opts.get<int>("value")) {
}

ConstEvaluator::~ConstEvaluator() {
}

int ConstEvaluator::compute_heuristic(const GlobalState &) {
    return value;
}

bool ConstEvaluator::is_dead_end() const {
    return false;
}

bool ConstEvaluator::dead_end_is_reliable() const {
    return true;
}

static Heuristic *_parse(OptionParser &parser) {
    parser.document_synopsis("Constant evaluator",
                             "Returns a constant value.");
    parser.add_option<int>("value", "the constant value", "1");
    Heuristic::add_options_to_parser(parser);
    Options opts = parser.parse();
    if (parser.dry_run()) {
        return 0;
    } else {
        if (opts.get<int>("values") < 0) {
            parser.error("value must be a value >= 0");
        }
        return new ConstEvaluator(opts);
    }
}

static Plugin<Heuristic> _plugin("const", _parse);
