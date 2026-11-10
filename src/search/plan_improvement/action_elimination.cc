#include "action_elimination.h"

#include "../operator_id.h"
#include "../state_registry.h"
#include "../task_proxy.h"

#include "../plugins/plugin.h"
#include "../task_utils/task_properties.h"
#include "../utils/markup.h"

#include <iostream>
#include <set>

using namespace std;
using namespace task_properties;

namespace action_elimination {
Plan ActionElimination::improve(
    const Plan &plan, const TaskProxy &task_proxy, StateRegistry &) {
    Plan reduced_plan = plan;
    OperatorsProxy operators = task_proxy.get_operators();
    State initial_state = task_proxy.get_initial_state();
    set<size_t> marked_actions;
    State prefix_state = initial_state;
    size_t i = 0;
    while (i < reduced_plan.size()) {
        marked_actions.clear();
        reduction_attempts++;
        State current_state = prefix_state;
        OperatorProxy prefix_operator = operators[reduced_plan.at(i)];
        marked_actions.insert(i);
        for (size_t j = i + 1; j < reduced_plan.size(); j++) {
            OperatorProxy current_operator = operators[reduced_plan.at(j)];
            if (is_applicable(current_operator, current_state)) {
                current_state =
                    current_state.get_unregistered_successor(current_operator);
            } else {
                marked_actions.insert(j);
            }
        }
        if (is_goal_state(task_proxy, current_state)) {
            successfull_reductions++;
            Plan improved_plan;
            for (size_t i = 0; i < reduced_plan.size(); i++) {
                if (marked_actions.find(i) == marked_actions.end()) {
                    improved_plan.push_back(reduced_plan[i]);
                }
            }
            reduced_plan = improved_plan;
        } else {
            prefix_state =
                prefix_state.get_unregistered_successor(prefix_operator);
            i++;
        }
    }
    return reduced_plan;
}

void ActionElimination::print_statistics() const {
    utils::g_log << "Action Elimination statistics:" << endl;
    utils::g_log << "Reduction attempts: " << reduction_attempts << endl;
    utils::g_log << "Successful reductions: " << successfull_reductions << endl;
}

class ActionEliminationFeature : public plugins::TypedFeature<PlanImprovement> {
public:
    ActionEliminationFeature() : TypedFeature("action_elimination") {
        document_title("Action Elimination");
        document_synopsis(
            "The algorithm was introduced in the following paper:" +
            utils::format_conference_reference(
                {"Hootan Nakhost ", " Martin Müller"},
                "Action Elimination and Plan Neighborhood Graph Search: Two Algorithms for Plan Improvement",
                "https://ai.dmi.unibas.ch/research/reading_group/nakhost-mueller-icaps2010.pdf",
                "Proceedings of the 20th International Conference on Automated Planning and Scheduling (ICAPS 2010)",
                "121-128", "AAAI Press", "2010"));
    }
    std::shared_ptr<PlanImprovement> create_component(
        const plugins::Options &) const override {
        return std::make_shared<ActionElimination>();
    }
};

static plugins::FeaturePlugin<ActionEliminationFeature> _plugin;
}