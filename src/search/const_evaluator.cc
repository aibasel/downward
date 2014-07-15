#include "const_evaluator.h"
#include "option_parser.h"
#include "plugin.h"


ConstEvaluator::ConstEvaluator(const Options &opts)
    :value(opts.get<int>("value")){
}

ConstEvaluator::ConstEvaluator(const int value)
    :value(value){
}

ConstEvaluator::~ConstEvaluator() {
}



bool ConstEvaluator::is_dead_end() const {
    return false;
}

bool ConstEvaluator::dead_end_is_reliable() const {
    return true;
}

int ConstEvaluator::get_value() const {
    return value;
}

static ScalarEvaluator *_parse(OptionParser &parser) {
    parser.document_synopsis("const-value evaluator",
                             "Returns a constant value.");
    parser.add_option<int>("value","the constant value", "1");
    Options opts = parser.parse();
    if (parser.dry_run())
        return 0;
    else
        return new ConstEvaluator(opts);
}

static Plugin<ScalarEvaluator> _plugin("c", _parse);
