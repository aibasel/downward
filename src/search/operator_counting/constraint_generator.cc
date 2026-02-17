#include "constraint_generator.h"

#include "../plugins/plugin.h"

using namespace std;

namespace operator_counting {
ConstraintGenerator::ConstraintGenerator(
    const std::shared_ptr<AbstractTask> &task)
    : TaskSpecificComponent(task) {
}

void ConstraintGenerator::initialize_constraints(
    const shared_ptr<AbstractTask> &, lp::LinearProgram &) {
}

static class ConstraintGeneratorCategoryPlugin
    : public plugins::TypedCategoryPlugin<TaskIndependentConstraintGenerator> {
public:
    ConstraintGeneratorCategoryPlugin()
        : TypedCategoryPlugin("ConstraintGenerator") {
        // TODO: Replace empty string by synopsis for the wiki page.
        document_synopsis(
            "This page describes different types of constraints and their corresponding generator. "
            "Operator-counting constraints are linear constraints that are satisfied by all plans. "
            "They are defined over variables that represent the number of times each operator is used.  "
            "An operator-counting heuristic minimizes the total cost of used operators subject to a set of such constraints");
    }
} _category_plugin;
}
