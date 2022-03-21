#include "pattern_collection_generator_manual.h"

#include "validation.h"

#include "../option_parser.h"
#include "../plugin.h"
#include "../task_proxy.h"

#include "../utils/logging.h"

#include <iostream>

using namespace std;

namespace pdbs {
PatternCollectionGeneratorManual::PatternCollectionGeneratorManual(const Options &opts)
    : PatternCollectionGenerator(opts),
      patterns(make_shared<PatternCollection>(opts.get_list<Pattern>("patterns"))) {
}

string PatternCollectionGeneratorManual::name() const {
    return "manual pattern collection generator";
}

PatternCollectionInformation PatternCollectionGeneratorManual::compute_patterns(
    const shared_ptr<AbstractTask> &task) {
    if (log.is_at_least_normal()) {
        log << "Manual pattern collection: " << *patterns << endl;
    }
    TaskProxy task_proxy(*task);
    return PatternCollectionInformation(task_proxy, patterns, log);
}

static shared_ptr<PatternCollectionGenerator> _parse(OptionParser &parser) {
    parser.add_list_option<Pattern>(
        "patterns",
        "list of patterns (which are lists of variable numbers of the planning "
        "task).");
    add_generator_options_to_parser(parser);

    Options opts = parser.parse();
    if (parser.dry_run())
        return nullptr;

    return make_shared<PatternCollectionGeneratorManual>(opts);
}

static Plugin<PatternCollectionGenerator> _plugin("manual_patterns", _parse);
}
