#include "pattern_generator_manual.h"

#include "pattern_information.h"

#include "../option_parser.h"
#include "../plugin.h"
#include "../task_proxy.h"

#include "../utils/logging.h"

#include <iostream>

using namespace std;

namespace pdbs {
PatternGeneratorManual::PatternGeneratorManual(const Options &opts)
    : PatternGenerator(opts), pattern(opts.get_list<int>("pattern")) {
}

string PatternGeneratorManual::name() const {
    return "manual pattern generator";
}

PatternInformation PatternGeneratorManual::compute_pattern(
    const shared_ptr<AbstractTask> &task) {
    PatternInformation pattern_info(TaskProxy(*task), move(pattern));
    if (log.is_at_least_normal()) {
        log << "Manual pattern: " << pattern_info.get_pattern() << endl;
    }
    return pattern_info;
}

static shared_ptr<PatternGenerator> _parse(OptionParser &parser) {
    parser.add_list_option<int>(
        "pattern",
        "list of variable numbers of the planning task that should be used as "
        "pattern.");
    add_generator_options_to_parser(parser);

    Options opts = parser.parse();
    if (parser.dry_run())
        return nullptr;

    return make_shared<PatternGeneratorManual>(opts);
}

static Plugin<PatternGenerator> _plugin("manual_pattern", _parse);
}
