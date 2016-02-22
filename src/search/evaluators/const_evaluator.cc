#include "const_evaluator.h"

#include "../option_parser.h"
#include "../plugin.h"

namespace const_evaluator {
ConstEvaluator::ConstEvaluator(const Options &opts)
    : Heuristic(opts),
      value(opts.get<int>("value")) {
}

int ConstEvaluator::compute_heuristic(const GlobalState &) {
    return value;
}

static Heuristic *_parse(OptionParser &parser) {
    parser.document_synopsis(
        "Constant evaluator",
        "Returns a constant value.");
    parser.add_option<int>(
        "value",
        "the constant value",
        "1",
        Bounds("0", "infinity"));
    parser.document_property("admissible", "no");
    parser.document_property("consistent", "yes");
    parser.document_property("safe", "no");
    parser.document_property("preferred operators", "no");
    Heuristic::add_options_to_parser(parser);
    Options opts = parser.parse();

    if (parser.dry_run())
        return nullptr;
    else
        return new ConstEvaluator(opts);
}

static Plugin<Heuristic> _plugin("const", _parse);
}
