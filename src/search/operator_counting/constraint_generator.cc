#include "constraint_generator.h"

#include "../plugin.h"

namespace operator_counting {
void ConstraintGenerator::initialize_constraints(
    const std::shared_ptr<AbstractTask> &,
    std::vector<lp::LPConstraint> &,
    double) {
}

static PluginTypePlugin<ConstraintGenerator> _type_plugin(
    "ConstraintGenerator",
    // TODO: Replace empty string by synopsis for the wiki page.
    "");
}
