#include "decompositions.h"

#include "utils.h"
#include "utils_landmarks.h"

#include "../option_parser.h"
#include "../plugin.h"
#include "../task_tools.h"

#include "../heuristics/additive_heuristic.h"

#include "../landmarks/landmark_graph.h"

#include "../tasks/domain_abstracted_task_factory.h"
#include "../tasks/modified_goals_task.h"

#include "../utils/rng.h"

#include <algorithm>
#include <cassert>
#include <iostream>
#include <string>
#include <unordered_set>
#include <vector>

using namespace std;

namespace CEGAR {
class SortFactsByIncreasingHaddValues {
    // Can't store as unique_ptr since the class needs copy-constructor.
    std::shared_ptr<AdditiveHeuristic::AdditiveHeuristic> hadd;

    int get_cost(Fact fact) {
        return hadd->get_cost_for_cegar(fact.first, fact.second);
    }

public:
    explicit SortFactsByIncreasingHaddValues(
        const std::shared_ptr<AbstractTask> &task)
        : hadd(get_additive_heuristic(task)) {
        TaskProxy task_proxy(*task);
        hadd->initialize_and_compute_heuristic_for_cegar(
            task_proxy.get_initial_state());
    }

    bool operator()(Fact a, Fact b) {
        return get_cost(a) < get_cost(b);
    }
};


static void remove_initial_state_facts(
    const TaskProxy &task_proxy, Facts &facts) {
    State initial_state = task_proxy.get_initial_state();
    facts.erase(remove_if(facts.begin(), facts.end(), [&](Fact fact) {
            return initial_state[fact.first].get_value() == fact.second;
        }), facts.end());
}

static void order_facts(
    const shared_ptr<AbstractTask> &task,
    SubtaskOrder subtask_order,
    vector<Fact> &facts) {
    cout << "Sort " << facts.size() << " facts" << endl;
    switch (subtask_order) {
    case SubtaskOrder::ORIGINAL:
        // Nothing to do.
        break;
    case SubtaskOrder::RANDOM:
        g_rng.shuffle(facts);
        break;
    case SubtaskOrder::HADD_UP:
    case SubtaskOrder::HADD_DOWN:
        sort(facts.begin(), facts.end(), SortFactsByIncreasingHaddValues(task));
        if (subtask_order == SubtaskOrder::HADD_DOWN)
            reverse(facts.begin(), facts.end());
        break;
    default:
        cerr << "Invalid task order: " << static_cast<int>(subtask_order) << endl;
        Utils::exit_with(Utils::ExitCode::INPUT_ERROR);
    }
}

static Facts filter_and_order_facts(
    const shared_ptr<AbstractTask> &task,
    SubtaskOrder subtask_order,
    Facts &facts) {
    TaskProxy task_proxy(*task);
    remove_initial_state_facts(task_proxy, facts);
    order_facts(task, subtask_order, facts);
    return facts;
}


NoDecomposition::NoDecomposition(const Options &opts)
    : num_copies(opts.get<int>("copies")) {
}

SharedTasks NoDecomposition::get_subtasks(
    const shared_ptr<AbstractTask> &task) const {
    SharedTasks subtasks;
    for (int i = 0; i < num_copies; ++i) {
        subtasks.push_back(task);
    }
    return subtasks;
}

GoalDecomposition::GoalDecomposition(const Options &opts)
    : subtask_order(SubtaskOrder(opts.get_enum("order"))) {
}

Facts GoalDecomposition::get_goal_facts(const TaskProxy &task_proxy) const {
    Facts facts;
    for (FactProxy goal : task_proxy.get_goals()) {
        facts.emplace_back(goal.get_variable().get_id(), goal.get_value());
    }
    return facts;
}

SharedTasks GoalDecomposition::get_subtasks(
    const shared_ptr<AbstractTask> &task) const {
    SharedTasks subtasks;
    TaskProxy task_proxy(*task);
    Facts goal_facts = get_goal_facts(task_proxy);
    filter_and_order_facts(task, subtask_order, goal_facts);
    for (Fact goal : goal_facts) {
        shared_ptr<AbstractTask> subtask =
            make_shared<ExtraTasks::ModifiedGoalsTask>(task, Facts {goal});
        subtasks.push_back(subtask);
    }
    return subtasks;
}


LandmarkDecomposition::LandmarkDecomposition(const Options &opts)
    : subtask_order(SubtaskOrder(opts.get_enum("order"))),
      landmark_graph(get_landmark_graph()),
      combine_facts(opts.get<bool>("combine_facts")) {
}

shared_ptr<AbstractTask> LandmarkDecomposition::build_domain_abstracted_task(
    shared_ptr<AbstractTask> &parent, Fact fact) const {
    assert(combine_facts);
    ExtraTasks::VarToGroups value_groups;
    for (auto &pair : get_prev_landmarks(*landmark_graph, fact)) {
        int var = pair.first;
        vector<int> &group = pair.second;
        if (group.size() >= 2)
            value_groups[var].push_back(group);
    }
    return ExtraTasks::build_domain_abstracted_task(parent, value_groups);
}

SharedTasks LandmarkDecomposition::get_subtasks(
    const shared_ptr<AbstractTask> &task) const {
    SharedTasks subtasks;
    // TODO: Use landmark graph for task once the LM code supports tasks API.
    Facts landmark_facts = get_fact_landmarks(*landmark_graph);
    filter_and_order_facts(task, subtask_order, landmark_facts);
    for (Fact landmark : landmark_facts) {
        shared_ptr<AbstractTask> subtask =
            make_shared<ExtraTasks::ModifiedGoalsTask>(task, Facts {landmark});
        if (combine_facts) {
            subtask = build_domain_abstracted_task(subtask, landmark);
        }
        subtasks.push_back(subtask);
    }
    return subtasks;
}

static shared_ptr<Decomposition> _parse_no_decomposition(OptionParser &parser) {
    parser.add_option<int>(
        "copies",
        "number of task copies",
        "1",
        Bounds("1", "infinity"));
    Options opts = parser.parse();
    if (parser.dry_run())
        return nullptr;
    else
        return make_shared<NoDecomposition>(opts);
}

static void add_subtask_order_option(OptionParser &parser) {
    vector<string> subtask_orders;
    subtask_orders.push_back("ORIGINAL");
    subtask_orders.push_back("RANDOM");
    subtask_orders.push_back("HADD_UP");
    subtask_orders.push_back("HADD_DOWN");
    parser.add_enum_option(
        "order", subtask_orders, "subtask order", "HADD_DOWN");
}

static shared_ptr<Decomposition> _parse_goals(OptionParser &parser) {
    add_subtask_order_option(parser);
    Options opts = parser.parse();
    if (parser.dry_run())
        return nullptr;
    else
        return make_shared<GoalDecomposition>(opts);
}

static shared_ptr<Decomposition> _parse_landmarks(OptionParser &parser) {
    add_subtask_order_option(parser);
    parser.add_option<bool>(
        "combine_facts",
        "combine landmark facts with domain abstraction",
        "true");
    Options opts = parser.parse();
    if (parser.dry_run())
        return nullptr;
    else
        return make_shared<LandmarkDecomposition>(opts);
}

static PluginShared<Decomposition> _plugin_original(
    "no_decomposition", _parse_no_decomposition);
static PluginShared<Decomposition> _plugin_goals(
    "decomposition_by_goals", _parse_goals);
static PluginShared<Decomposition> _plugin_landmarks(
    "decomposition_by_landmarks", _parse_landmarks);

static PluginTypePlugin<Decomposition> _type_plugin(
    "Decomposition",
    "Task decomposition (used by the CEGAR heuristic).");
}
