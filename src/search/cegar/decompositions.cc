#include "decompositions.h"

#include "domain_abstracted_task.h"
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
    : task_proxy(*opts.get<TaskProxy *>("task_proxy")) {
}

Subtask Decomposition::get_original_task() const {
    return g_root_task();
}


NoDecomposition::NoDecomposition(const Options &opts)
    : Decomposition(opts),
      max_abstractions(opts.get<int>("max_abstractions")) {
}

Subtasks NoDecomposition::get_subtasks() const {
    Subtasks tasks;
    for (int i = 0; i < max_abstractions; ++i) {
        tasks.push_back(get_original_task());
    }
    return tasks;
}


FactDecomposition::FactDecomposition(const Options &opts)
    : Decomposition(opts),
      task_order(TaskOrder(opts.get_enum("task_order"))) {
}

void FactDecomposition::remove_intial_state_facts(Facts &facts) const {
    facts.erase(remove_if(facts.begin(), facts.end(),
        [&](Fact fact) {
            return task_proxy.get_initial_state()[fact.first].get_value() == fact.second;
        }),
        facts.end());
}

Facts FactDecomposition::get_filtered_and_ordered_facts() const {
    Facts facts = get_facts();
    remove_intial_state_facts(facts);
    order_facts(facts);
    return facts;
}

void FactDecomposition::order_facts(vector<Fact> &facts) const {
    assert(task_order != TaskOrder::ORIGINAL);
    cout << "Sort " << facts.size() << " facts" << endl;
    if (task_order == TaskOrder::MIXED) {
        g_rng.shuffle(facts);
    } else if (task_order == TaskOrder::HADD_UP || task_order == TaskOrder::HADD_DOWN) {
        sort(facts.begin(), facts.end(), SortHaddValuesUp(task_proxy));
        if (task_order == TaskOrder::HADD_DOWN)
            reverse(facts.begin(), facts.end());
    } else {
        cerr << "Invalid task ordering: " << static_cast<int>(task_order) << endl;
        exit_with(EXIT_INPUT_ERROR);
    }
}


GoalDecomposition::GoalDecomposition(const Options &opts)
    : FactDecomposition(opts) {
}

Facts GoalDecomposition::get_facts() const {
    Facts facts;
    for (FactProxy goal : task_proxy.get_goals()) {
        facts.push_back(get_raw_fact(goal));
    }
    return facts;
}

Subtasks GoalDecomposition::get_subtasks() const {
    Subtasks tasks;
    for (Fact goal : get_filtered_and_ordered_facts()) {
        Subtask abstracted_task = get_original_task();
        Facts goals {goal};
        abstracted_task = make_shared<ModifiedGoalsTask>(abstracted_task, goals);
        tasks.push_back(abstracted_task);
    }
    return tasks;
}


LandmarkDecomposition::LandmarkDecomposition(const Options &opts)
    : FactDecomposition(opts),
      landmark_graph(get_landmark_graph()) {

    if (DEBUG)
        dump_landmark_graph(landmark_graph);
    if (opts.get<bool>("write_graphs"))
        write_landmark_graph_dot_file(landmark_graph);
}

Subtask LandmarkDecomposition::get_domain_abstracted_task(Subtask parent, Fact fact) const {
    assert(combine_facts);
    VarToValues landmark_groups = get_prev_landmarks(landmark_graph, fact);
    VarToGroups groups;
    for (auto &pair : landmark_groups) {
        int var = pair.first;
        vector<int> &group = pair.second;
        if (group.size() >= 2)
            groups[var].push_back(group);
    }
    return make_shared<DomainAbstractedTask>(parent, groups);
}

Facts LandmarkDecomposition::get_facts() const {
    return get_fact_landmarks(landmark_graph);
}

Subtasks LandmarkDecomposition::get_subtasks() const {
    Subtasks subtasks;
    for (Fact landmark : get_filtered_and_ordered_facts()) {
        Subtask subtask = get_original_task();
        Facts goals {landmark};
        subtask = make_shared<ModifiedGoalsTask>(subtask, goals);
        if (combine_facts) {
            subtask = get_domain_abstracted_task(subtask, landmark);
        }
        subtasks.push_back(subtask);
    }
    return subtasks;
}
}
