#include "constraint_generator.h"

#include "../plugins/plugin.h"

using namespace std;

namespace operator_counting {
void ConstraintGenerator::initialize_constraints(
    const shared_ptr<AbstractTask> &, lp::LinearProgram &) {
}

static class ConstraintGeneratorCategoryPlugin : public plugins::TypedCategoryPlugin<ConstraintGenerator> {
public:
    ConstraintGeneratorCategoryPlugin() : TypedCategoryPlugin("ConstraintGenerator") {
        // TODO: Replace empty string by synopsis for the wiki page.
        document_synopsis("This page describes different types of constraints and their corresponding generator.");
    }
}
_category_plugin;
}
