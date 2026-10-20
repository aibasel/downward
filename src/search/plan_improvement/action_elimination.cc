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

Plan ActionElimination::improve(
    const Plan &plan, const TaskProxy &task_proxy,
    StateRegistry &state_registry) {
    Plan reduced_plan = plan;
    cout << "action_elimination is called\n";
    OperatorsProxy operators = task_proxy.get_operators();
    State initial_state = state_registry.get_initial_state();
    // Stores the indices of all marked actions.
    set<size_t> marked_actions;
    State prefix_state = initial_state;
    // AE: This is only true for action costs of 1. Remove later since its not
    // necessary. Didn't find a corresponding function rn.
    size_t plan_cost = plan.size();
    cout << "Plan size in the beginning: " << plan_cost << "\n";
    size_t i = 0;
    while (i < reduced_plan.size()) {
        marked_actions.clear();
        State current_state = prefix_state;
        OperatorProxy prefix_operator = operators[reduced_plan.at(i)];
        // cout << "Try to remove action and simulate rest of plan: " << i <<
        // "\n";
        marked_actions.insert(i);
        for (size_t j = i + 1; j < reduced_plan.size(); j++) {
            // cout << "Check first if a_j is applicable in current_state" <<
            // "\n";
            OperatorProxy current_operator = operators[reduced_plan.at(j)];
            if (is_applicable(current_operator, current_state)) {
                // cout << "Action was applicable, apply it and check next one"
                // << "\n";
                current_state = state_registry.get_successor_state(
                    current_state, current_operator);
            } else {
                // cout
                //<< "Action was not applicable, mark it and try the next one"
                //<< "\n";
                marked_actions.insert(j);
            }
        }
        if (is_goal_state(task_proxy, current_state)) {
            cout << "FOUND A REDUCTION! Remove marked actions from plan"
                 << "\n";
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
            /*cout
                << "unmark all actions and apply action i to get a new
               current state"
                << "\n";
            */
            i++;
        }
    }
    return reduced_plan;
}

class ActionEliminationFeature : public plugins::TypedFeature<PlanImprovement> {

public:
    ActionEliminationFeature() : TypedFeature("action_elimination") {
        document_title("Action Elimination");
        document_synopsis("Improves plans by removing unnecessary actions.");
    }

    std::shared_ptr<PlanImprovement> create_component(
        const plugins::Options &) const override {
        return std::make_shared<ActionElimination>();
    }
};

static plugins::FeaturePlugin<ActionEliminationFeature> _plugin;