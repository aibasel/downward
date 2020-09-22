#include "constraint_generator.h"

#include "../plugin.h"

using namespace std;

namespace operator_counting {
void ConstraintGenerator::initialize_constraints(
    const shared_ptr<AbstractTask> &, named_vector::NamedVector<lp::LPConstraint> &, double) {
}

static PluginTypePlugin<ConstraintGenerator> _type_plugin(
    "ConstraintGenerator",
    // TODO: Replace empty string by synopsis for the wiki page.
    "");
}
