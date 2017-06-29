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
    : patterns(make_shared<PatternCollection>(opts.get_list<Pattern>("patterns"))) {
}

PatternCollectionInformation PatternCollectionGeneratorManual::generate(
    const shared_ptr<AbstractTask> &task) {
    cout << "Manual pattern collection: " << *patterns << endl;
    TaskProxy task_proxy(*task);
    return PatternCollectionInformation(task_proxy, patterns);
}

static shared_ptr<PatternCollectionGenerator> _parse(OptionParser &parser) {
    parser.add_list_option<Pattern>(
        "patterns",
        "list of patterns (which are lists of variable numbers of the planning "
        "task).");

    Options opts = parser.parse();
    if (parser.dry_run())
        return nullptr;

    return make_shared<PatternCollectionGeneratorManual>(opts);
}

static PluginShared<PatternCollectionGenerator> _plugin("manual_patterns", _parse);
}
