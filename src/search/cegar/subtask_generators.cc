#include "subtask_generators.h"

#include "utils.h"
#include "utils_landmarks.h"

#include "../option_parser.h"
#include "../plugin.h"

#include "../heuristics/additive_heuristic.h"
#include "../landmarks/landmark_graph.h"
#include "../task_utils/task_properties.h"
#include "../tasks/domain_abstracted_task_factory.h"
#include "../tasks/modified_goals_task.h"
#include "../utils/rng.h"
#include "../utils/rng_options.h"

#include <algorithm>
#include <cassert>
#include <iostream>
#include <string>
#include <unordered_set>
#include <vector>

using namespace std;

namespace cegar {
class SortFactsByIncreasingHaddValues {
    // Can't store as unique_ptr since the class needs copy-constructor.
    shared_ptr<additive_heuristic::AdditiveHeuristic> hadd;

    int get_cost(const FactPair &fact) {
        return hadd->get_cost_for_cegar(fact.var, fact.value);
    }

public:
    explicit SortFactsByIncreasingHaddValues(
        const shared_ptr<AbstractTask> &task)
        : hadd(create_additive_heuristic(task)) {
        TaskProxy task_proxy(*task);
        hadd->compute_heuristic_for_cegar(task_proxy.get_initial_state());
    }

    bool operator()(const FactPair &a, const FactPair &b) {
        return get_cost(a) < get_cost(b);
    }
};


static void remove_initial_state_facts(
    const TaskProxy &task_proxy, Facts &facts) {
    State initial_state = task_proxy.get_initial_state();
    facts.erase(remove_if(facts.begin(), facts.end(), [&](FactPair fact) {
            return initial_state[fact.var].get_value() == fact.value;
        }), facts.end());
}

static void order_facts(
    const shared_ptr<AbstractTask> &task,
    FactOrder fact_order,
    vector<FactPair> &facts,
    utils::RandomNumberGenerator &rng) {
    cout << "Sort " << facts.size() << " facts" << endl;
    switch (fact_order) {
    case FactOrder::ORIGINAL:
        // Nothing to do.
        break;
    case FactOrder::RANDOM:
        rng.shuffle(facts);
        break;
    case FactOrder::HADD_UP:
    case FactOrder::HADD_DOWN:
        sort(facts.begin(), facts.end(), SortFactsByIncreasingHaddValues(task));
        if (fact_order == FactOrder::HADD_DOWN)
            reverse(facts.begin(), facts.end());
        break;
    default:
        cerr << "Invalid task order: " << static_cast<int>(fact_order) << endl;
        utils::exit_with(utils::ExitCode::INPUT_ERROR);
    }
}

static Facts filter_and_order_facts(
    const shared_ptr<AbstractTask> &task,
    FactOrder fact_order,
    Facts &facts,
    utils::RandomNumberGenerator &rng) {
    TaskProxy task_proxy(*task);
    remove_initial_state_facts(task_proxy, facts);
    order_facts(task, fact_order, facts, rng);
    return facts;
}


TaskDuplicator::TaskDuplicator(const Options &opts)
    : num_copies(opts.get<int>("copies")) {
}

SharedTasks TaskDuplicator::get_subtasks(
    const shared_ptr<AbstractTask> &task) const {
    SharedTasks subtasks;
    for (int i = 0; i < num_copies; ++i) {
        subtasks.push_back(task);
    }
    return subtasks;
}

GoalDecomposition::GoalDecomposition(const Options &opts)
    : fact_order(FactOrder(opts.get_enum("order"))),
      rng(utils::parse_rng_from_options(opts)) {
}

SharedTasks GoalDecomposition::get_subtasks(
    const shared_ptr<AbstractTask> &task) const {
    SharedTasks subtasks;
    TaskProxy task_proxy(*task);
    Facts goal_facts = task_properties::get_fact_pairs(task_proxy.get_goals());
    filter_and_order_facts(task, fact_order, goal_facts, *rng);
    for (const FactPair &goal : goal_facts) {
        shared_ptr<AbstractTask> subtask =
            make_shared<extra_tasks::ModifiedGoalsTask>(task, Facts {goal});
        subtasks.push_back(subtask);
    }
    return subtasks;
}


LandmarkDecomposition::LandmarkDecomposition(const Options &opts)
    : fact_order(FactOrder(opts.get_enum("order"))),
      combine_facts(opts.get<bool>("combine_facts")),
      rng(utils::parse_rng_from_options(opts)) {
}

shared_ptr<AbstractTask> LandmarkDecomposition::build_domain_abstracted_task(
    const shared_ptr<AbstractTask> &parent,
    const landmarks::LandmarkGraph &landmark_graph,
    const FactPair &fact) const {
    assert(combine_facts);
    extra_tasks::VarToGroups value_groups;
    for (auto &pair : get_prev_landmarks(landmark_graph, fact)) {
        int var = pair.first;
        vector<int> &group = pair.second;
        if (group.size() >= 2)
            value_groups[var].push_back(group);
    }
    return extra_tasks::build_domain_abstracted_task(parent, value_groups);
}

SharedTasks LandmarkDecomposition::get_subtasks(
    const shared_ptr<AbstractTask> &task) const {
    SharedTasks subtasks;
    shared_ptr<landmarks::LandmarkGraph> landmark_graph =
        get_landmark_graph(task);
    Facts landmark_facts = get_fact_landmarks(*landmark_graph);
    filter_and_order_facts(task, fact_order, landmark_facts, *rng);
    for (const FactPair &landmark : landmark_facts) {
        shared_ptr<AbstractTask> subtask =
            make_shared<extra_tasks::ModifiedGoalsTask>(task, Facts {landmark});
        if (combine_facts) {
            subtask = build_domain_abstracted_task(
                subtask, *landmark_graph, landmark);
        }
        subtasks.push_back(subtask);
    }
    return subtasks;
}

static shared_ptr<SubtaskGenerator> _parse_original(OptionParser &parser) {
    parser.add_option<int>(
        "copies",
        "number of task copies",
        "1",
        Bounds("1", "infinity"));
    Options opts = parser.parse();
    if (parser.dry_run())
        return nullptr;
    else
        return make_shared<TaskDuplicator>(opts);
}

static void add_fact_order_option(OptionParser &parser) {
    vector<string> fact_orders;
    fact_orders.push_back("ORIGINAL");
    fact_orders.push_back("RANDOM");
    fact_orders.push_back("HADD_UP");
    fact_orders.push_back("HADD_DOWN");
    parser.add_enum_option(
        "order",
        fact_orders,
        "ordering of goal or landmark facts",
        "HADD_DOWN");
    utils::add_rng_options(parser);
}

static shared_ptr<SubtaskGenerator> _parse_goals(OptionParser &parser) {
    add_fact_order_option(parser);
    Options opts = parser.parse();
    if (parser.dry_run())
        return nullptr;
    else
        return make_shared<GoalDecomposition>(opts);
}

static shared_ptr<SubtaskGenerator> _parse_landmarks(OptionParser &parser) {
    add_fact_order_option(parser);
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

static PluginShared<SubtaskGenerator> _plugin_original(
    "original", _parse_original);
static PluginShared<SubtaskGenerator> _plugin_goals(
    "goals", _parse_goals);
static PluginShared<SubtaskGenerator> _plugin_landmarks(
    "landmarks", _parse_landmarks);

static PluginTypePlugin<SubtaskGenerator> _type_plugin(
    "SubtaskGenerator",
    "Subtask generator (used by the CEGAR heuristic).");
}
