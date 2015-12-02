#include "pattern_collection_generator_manual.h"

#include "../option_parser.h"
#include "../plugin.h"
#include "../task_proxy.h"

#include "validation.h"

#include <iostream>

using namespace std;


PatternCollectionGeneratorManual::PatternCollectionGeneratorManual(const Options &opts)
    : patterns(make_shared<PatternCollection>(opts.get_list<Pattern>("patterns"))) {
}

PatternCollectionInformation PatternCollectionGeneratorManual::generate(
    std::shared_ptr<AbstractTask> task) {
    TaskProxy task_proxy(*task);
    validate_and_normalize_patterns(task_proxy, *patterns);
    cout << "Manual pattern collection: " << *patterns << endl;
    return PatternCollectionInformation(task, patterns);
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

static PluginShared<PatternCollectionGenerator> _plugin("manual", _parse);
