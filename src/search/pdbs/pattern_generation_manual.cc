#include "pattern_generation_manual.h"

#include "../option_parser.h"
#include "../plugin.h"

#include "util.h"

#include <vector>

using namespace std;

PatternGenerationManual::PatternGenerationManual(const Options &opts)
    : patterns(make_shared<Patterns>(opts.get_list<Pattern>("patterns"))) {
}

PatternCollection PatternGenerationManual::generate(std::shared_ptr<AbstractTask> task) {
    TaskProxy task_proxy(*task);
    validate_and_normalize_patterns(task_proxy, *patterns);
    cout << "Manual pattern collection: " << *patterns << endl;
    return PatternCollection(task, patterns);
}

static shared_ptr<PatternCollectionGenerator> _parse(OptionParser &parser) {
    parser.add_list_option<Pattern>(
        "patterns",
        "list of patterns (which are lists of variable numbers of the planning "
        "task).",
        OptionParser::NONE);

    Options opts = parser.parse();
    if (parser.dry_run())
        return nullptr;

    return make_shared<PatternGenerationManual>(opts);
}

static PluginShared<PatternCollectionGenerator> _plugin("manual", _parse);
