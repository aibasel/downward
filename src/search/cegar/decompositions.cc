#include "decompositions.h"

#include "utils_landmarks.h"

#include "../option_parser.h"
#include "../plugin.h"
#include "../rng.h"
#include "../task_tools.h"

#include "../tasks/domain_abstracted_task_factory.h"
#include "../tasks/modified_goals_task.h"

#include <algorithm>
#include <cassert>
#include <iostream>
#include <string>
#include <unordered_set>
#include <vector>

using namespace std;

namespace CEGAR {
NoDecomposition::NoDecomposition(const Options &opts)
    : num_copies(opts.get<int>("copies")) {
}

Tasks NoDecomposition::get_subtasks(const Task &task) const {
    Tasks subtasks;
    for (int i = 0; i < num_copies; ++i) {
        subtasks.push_back(task);
    }
    return subtasks;
}


FactDecomposition::FactDecomposition(const Options &opts)
    : subtask_order(SubtaskOrder(opts.get_enum("order"))) {
}

void FactDecomposition::remove_initial_state_facts(
    const TaskProxy &task_proxy, Facts &facts) const {
    State initial_state = task_proxy.get_initial_state();
    facts.erase(remove_if(facts.begin(), facts.end(), [&](Fact fact) {
            return initial_state[fact.first].get_value() == fact.second;
        }), facts.end());
}

Facts FactDecomposition::get_filtered_and_ordered_facts(const Task &task) const {
    TaskProxy task_proxy(*task);
    Facts facts = get_facts(task_proxy);
    remove_initial_state_facts(task_proxy, facts);
    order_facts(task, facts);
    return facts;
}

void FactDecomposition::order_facts(const Task &task, vector<Fact> &facts) const {
    cout << "Sort " << facts.size() << " facts" << endl;
    if (subtask_order == SubtaskOrder::ORIGINAL) {
        // Nothing to do.
    } else if (subtask_order == SubtaskOrder::RANDOM) {
        g_rng.shuffle(facts);
    } else if (subtask_order == SubtaskOrder::HADD_UP ||
               subtask_order == SubtaskOrder::HADD_DOWN) {
        sort(facts.begin(), facts.end(),
             SortHaddValuesUp(get_additive_heuristic(task)));
        if (subtask_order == SubtaskOrder::HADD_DOWN)
            reverse(facts.begin(), facts.end());
    } else {
        cerr << "Invalid task order: " << static_cast<int>(subtask_order) << endl;
        exit_with(EXIT_INPUT_ERROR);
    }
}


GoalDecomposition::GoalDecomposition(const Options &opts)
    : FactDecomposition(opts) {
}

Facts GoalDecomposition::get_facts(const TaskProxy &task_proxy) const {
    Facts facts;
    for (FactProxy goal : task_proxy.get_goals()) {
        facts.emplace_back(goal.get_variable().get_id(), goal.get_value());
    }
    return facts;
}

Tasks GoalDecomposition::get_subtasks(const Task &task) const {
    Tasks subtasks;
    for (Fact goal : get_filtered_and_ordered_facts(task)) {
        Task subtask = make_shared<tasks::ModifiedGoalsTask>(task, Facts {goal});
        subtasks.push_back(subtask);
    }
    return subtasks;
}


LandmarkDecomposition::LandmarkDecomposition(const Options &opts)
    : FactDecomposition(opts),
      landmark_graph(get_landmark_graph()),
      combine_facts(opts.get<bool>("combine_facts")) {
    if (DEBUG)
        dump_landmark_graph(landmark_graph);
    if (opts.get<bool>("write_graph"))
        write_landmark_graph_dot_file(landmark_graph);
}

Task LandmarkDecomposition::get_domain_abstracted_task(
    Task parent, Fact fact) const {
    assert(combine_facts);
    tasks::VarToGroups value_groups;
    for (auto &pair : get_prev_landmarks(landmark_graph, fact)) {
        int var = pair.first;
        vector<int> &group = pair.second;
        if (group.size() >= 2)
            value_groups[var].push_back(group);
    }
    return tasks::build_domain_abstracted_task(parent, value_groups);
}

Facts LandmarkDecomposition::get_facts(const TaskProxy &) const {
    // TODO: Use landmark graph for task once the LM code supports tasks API.
    return get_fact_landmarks(landmark_graph);
}

Tasks LandmarkDecomposition::get_subtasks(const Task &task) const {
    Tasks subtasks;
    for (Fact landmark : get_filtered_and_ordered_facts(task)) {
        Task subtask = make_shared<tasks::ModifiedGoalsTask>(task, Facts {landmark});
        if (combine_facts) {
            subtask = get_domain_abstracted_task(subtask, landmark);
        }
        subtasks.push_back(subtask);
    }
    return subtasks;
}

static shared_ptr<Decomposition> _parse_original(OptionParser &parser) {
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

static void add_common_fact_decomposition_options(OptionParser &parser) {
    vector<string> subtask_orders;
    subtask_orders.push_back("ORIGINAL");
    subtask_orders.push_back("RANDOM");
    subtask_orders.push_back("HADD_UP");
    subtask_orders.push_back("HADD_DOWN");
    parser.add_enum_option(
        "order", subtask_orders, "subtask order", "HADD_DOWN");
}

static shared_ptr<Decomposition> _parse_goals(OptionParser &parser) {
    add_common_fact_decomposition_options(parser);
    Options opts = parser.parse();
    if (parser.dry_run())
        return nullptr;
    else
        return make_shared<GoalDecomposition>(opts);
}

static shared_ptr<Decomposition> _parse_landmarks(OptionParser &parser) {
    add_common_fact_decomposition_options(parser);
    parser.add_option<bool>(
        "combine_facts",
        "combine landmark facts with domain abstraction",
        "true");
    parser.add_option<bool>(
        "write_graph",
        "write dot file for landmark graph",
        "false");
    Options opts = parser.parse();
    if (parser.dry_run())
        return nullptr;
    else
        return make_shared<LandmarkDecomposition>(opts);
}

static PluginShared<Decomposition> _plugin_original("no_decomposition", _parse_original);
static PluginShared<Decomposition> _plugin_goals("decomposition_by_goals", _parse_goals);
static PluginShared<Decomposition> _plugin_landmarks("decomposition_by_landmarks", _parse_landmarks);

static PluginTypePlugin<Decomposition> _type_plugin(
    "Decomposition",
    "Task decomposition (used by the CEGAR heuristic).");
}
