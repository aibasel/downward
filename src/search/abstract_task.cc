#include "abstract_task.h"

#include "per_task_information.h"

#include "plugins/plugin.h"

#include <iostream>

using namespace std;

const FactPair FactPair::no_fact = FactPair(-1, -1);

ostream &operator<<(ostream &os, const FactPair &fact_pair) {
    os << fact_pair.var << "=" << fact_pair.value;
    return os;
}

static class AbstractTaskCategoryPlugin : public plugins::TypedCategoryPlugin<AbstractTask> {
public:
    AbstractTaskCategoryPlugin() : TypedCategoryPlugin("AbstractTask") {
        // TODO: Replace empty string by synopsis for the wiki page.
        document_synopsis("");
    }
}
_category_plugin;
