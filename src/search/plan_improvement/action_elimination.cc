#include "action_elimination.h"

#include "../operator_id.h"
#include "../state_registry.h"
#include "../task_proxy.h"

#include "../plugins/plugin.h"
#include "../task_utils/task_properties.h"

#include <iostream>
#include <set>

using namespace std;
using namespace task_properties;

// TODO: Check all the names if they make sense.
// TODO: Check how I should do/add comments.
// TODO: Do all the datatypes make sense? For example: Is set insertion O(1)
namespace action_elimination {
Plan ActionElimination::improve(
    const Plan &plan, const TaskProxy &task_proxy,
    StateRegistry &state_registry) {
    Plan reduced_plan = plan;
    OperatorsProxy operators = task_proxy.get_operators();
    State initial_state = state_registry.get_initial_state();
    set<size_t> marked_actions;
    State prefix_state = initial_state;
    // TODO: Add here some output that indicates the cost before improvement.
    size_t plan_cost = calculate_plan_cost(plan, task_proxy);
    cout << "Plan cost in the beginning: " << plan_cost << "\n";
    size_t i = 0;
    while (i < reduced_plan.size()) {
        marked_actions.clear();
        State current_state = prefix_state;
        OperatorProxy prefix_operator = operators[reduced_plan.at(i)];
        marked_actions.insert(i);
        for (size_t j = i + 1; j < reduced_plan.size(); j++) {
            OperatorProxy current_operator = operators[reduced_plan.at(j)];
            if (is_applicable(current_operator, current_state)) {
                // TODO: This will add newly discovered states to the state
                // registry, even though its not needed.
                current_state = state_registry.get_successor_state(
                    current_state, current_operator);
            } else {
                marked_actions.insert(j);
            }
        }
        if (is_goal_state(task_proxy, current_state)) {
            // TODO: Should there be output for each reduction found? Or only at
            // the end the total reduced plan?
            // TODO: Make this more efficient? Its O(n) work but only if a
            // reduction is found. Not sure its worth it to increase the
            // simulation effort to reduce work here.
            Plan improved_plan;
            for (size_t i = 0; i < reduced_plan.size(); i++) {
                if (marked_actions.find(i) == marked_actions.end()) {
                    improved_plan.push_back(reduced_plan[i]);
                }
            }
            reduced_plan = improved_plan;
        } else {
            prefix_state = state_registry.get_successor_state(
                prefix_state, prefix_operator);
            i++;
        }
    }
    return reduced_plan;
}

class ActionEliminationFeature : public plugins::TypedFeature<PlanImprovement> {
    // TODO: Add reference to Nahkost and Müller here.
public:
    ActionEliminationFeature() : TypedFeature("action_elimination") {
        document_title("Action Elimination");
        document_synopsis("Improves plans by removing unnecessary actions.");
    }

    // TODO: Removed the option opts currently, since there was a fd build
    // warning.
    std::shared_ptr<PlanImprovement> create_component(
        const plugins::Options &) const override {
        return std::make_shared<ActionElimination>();
    }
};

static plugins::FeaturePlugin<ActionEliminationFeature> _plugin;
}