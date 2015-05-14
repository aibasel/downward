#include "decompositions.h"

#include "domain_abstracted_task_builder.h"
#include "modified_goals_task.h"
#include "utils_landmarks.h"

#include "../option_parser.h"
#include "../plugin.h"
#include "../task_tools.h"

#include <algorithm>
#include <cassert>
#include <iostream>
#include <string>
#include <unordered_set>
#include <vector>

using namespace std;

namespace cegar {
Decomposition::Decomposition(const Options &opts)
    : task(get_task_from_options(opts)),
      task_proxy(*task) {
}

Subtask Decomposition::get_original_task() const {
    return task;
}


NoDecomposition::NoDecomposition(const Options &opts)
    : Decomposition(opts),
      num_copies(opts.get<int>("copies")) {
}

Subtasks NoDecomposition::get_subtasks() const {
    Subtasks tasks;
    for (int i = 0; i < num_copies; ++i) {
        tasks.push_back(get_original_task());
    }
    return tasks;
}


FactDecomposition::FactDecomposition(const Options &opts)
    : Decomposition(opts),
      subtask_order(SubtaskOrder(opts.get_enum("order"))) {
}

void FactDecomposition::remove_initial_state_facts(Facts &facts) const {
    facts.erase(remove_if(
                    facts.begin(), facts.end(),
                    [&](Fact fact) {
                        return task_proxy.get_initial_state()[fact.first].get_value() == fact.second;
                    }
                    ),
                facts.end());
}

Facts FactDecomposition::get_filtered_and_ordered_facts() const {
    Facts facts = get_facts();
    remove_initial_state_facts(facts);
    order_facts(facts);
    return facts;
}

void FactDecomposition::order_facts(vector<Fact> &facts) const {
    assert(subtask_order != SubtaskOrder::ORIGINAL);
    cout << "Sort " << facts.size() << " facts" << endl;
    if (subtask_order == SubtaskOrder::MIXED) {
        g_rng.shuffle(facts);
    } else if (subtask_order == SubtaskOrder::HADD_UP ||
               subtask_order == SubtaskOrder::HADD_DOWN) {
        sort(facts.begin(), facts.end(),
             SortHaddValuesUp(get_additive_heuristic(get_original_task())));
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

Facts GoalDecomposition::get_facts() const {
    Facts facts;
    for (FactProxy goal : task_proxy.get_goals()) {
        facts.emplace_back(goal.get_variable().get_id(), goal.get_value());
    }
    return facts;
}

Subtasks GoalDecomposition::get_subtasks() const {
    Subtasks tasks;
    for (Fact goal : get_filtered_and_ordered_facts()) {
        Subtask abstracted_task = get_original_task();
        Facts goals {
            goal
        };
        abstracted_task = make_shared<ModifiedGoalsTask>(abstracted_task, goals);
        tasks.push_back(abstracted_task);
    }
    return tasks;
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

Subtask LandmarkDecomposition::get_domain_abstracted_task(
    Subtask parent, Fact fact) const {
    assert(combine_facts);
    VarToGroups value_groups;
    for (auto &pair : get_prev_landmarks(landmark_graph, fact)) {
        int var = pair.first;
        vector<int> &group = pair.second;
        if (group.size() >= 2)
            value_groups[var].push_back(group);
    }
    return build_domain_abstracted_task(parent, value_groups);
}

Facts LandmarkDecomposition::get_facts() const {
    return get_fact_landmarks(landmark_graph);
}

Subtasks LandmarkDecomposition::get_subtasks() const {
    Subtasks subtasks;
    for (Fact landmark : get_filtered_and_ordered_facts()) {
        Subtask subtask = get_original_task();
        Facts goals {
            landmark
        };
        subtask = make_shared<ModifiedGoalsTask>(subtask, goals);
        if (combine_facts) {
            subtask = get_domain_abstracted_task(subtask, landmark);
        }
        subtasks.push_back(subtask);
    }
    return subtasks;
}

static shared_ptr<Decomposition> _parse_original(OptionParser &parser) {
    Heuristic::add_options_to_parser(parser);
    parser.add_option<int>("copies", "number of task copies", "1");
    Options opts = parser.parse();
    if (parser.dry_run())
        return nullptr;
    else
        return make_shared<NoDecomposition>(opts);
}

static void add_common_fact_decomposition_options(OptionParser &parser) {
    Heuristic::add_options_to_parser(parser);
    vector<string> subtask_orders;
    subtask_orders.push_back("ORIGINAL");
    subtask_orders.push_back("MIXED");
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
    parser.add_option<bool>("combine_facts", "combine landmark facts", "true");
    parser.add_option<bool>("write_graph", "write landmark graph dot file", "false");
    Options opts = parser.parse();
    if (parser.dry_run())
        return nullptr;
    else
        return make_shared<LandmarkDecomposition>(opts);
}

static PluginShared<Decomposition> _plugin_original("original", _parse_original);
static PluginShared<Decomposition> _plugin_goals("goals", _parse_goals);
static PluginShared<Decomposition> _plugin_landmarks("landmarks", _parse_landmarks);
}
