#include "pattern_generation_single_manual.h"

#include "util.h"

#include "../option_parser.h"
#include "../plugin.h"
#include "../task_proxy.h"

#include <iostream>

using namespace std;


PatternGenerationSingleManual::PatternGenerationSingleManual(const Options &opts)
    : pattern(opts.get_list<int>("pattern")) {
}

Pattern PatternGenerationSingleManual::generate(shared_ptr<AbstractTask> task) {
    TaskProxy task_proxy(*task);
    validate_and_normalize_pattern(task_proxy, pattern);
    cout << "Manual pattern: " << pattern << endl;
    return pattern;
}

static shared_ptr<PatternGenerator> _parse(OptionParser &parser) {
    parser.add_list_option<int>(
        "pattern",
        "list of variable numbers of the planning task that should be used as "
        "pattern.",
        OptionParser::NONE);

    Options opts = parser.parse();
    if (parser.dry_run())
        return nullptr;

    return make_shared<PatternGenerationSingleManual>(opts);
}

static PluginShared<PatternGenerator> _plugin("manual", _parse);
