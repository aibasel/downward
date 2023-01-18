#include "subtask_generators.h"

#include "utils.h"
#include "utils_landmarks.h"

#include "../heuristics/additive_heuristic.h"
#include "../landmarks/landmark_graph.h"
#include "../plugins/plugin.h"
#include "../task_utils/task_properties.h"
#include "../tasks/domain_abstracted_task_factory.h"
#include "../tasks/modified_goals_task.h"
#include "../utils/logging.h"
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
    utils::RandomNumberGenerator &rng,
    utils::LogProxy &log) {
    if (log.is_at_least_verbose()) {
        log << "Sort " << facts.size() << " facts" << endl;
    }
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
        utils::exit_with(utils::ExitCode::SEARCH_INPUT_ERROR);
    }
}

static Facts filter_and_order_facts(
    const shared_ptr<AbstractTask> &task,
    FactOrder fact_order,
    Facts &facts,
    utils::RandomNumberGenerator &rng,
    utils::LogProxy &log) {
    TaskProxy task_proxy(*task);
    remove_initial_state_facts(task_proxy, facts);
    order_facts(task, fact_order, facts, rng, log);
    return facts;
}


TaskDuplicator::TaskDuplicator(const plugins::Options &opts)
    : num_copies(opts.get<int>("copies")) {
}

SharedTasks TaskDuplicator::get_subtasks(
    const shared_ptr<AbstractTask> &task, utils::LogProxy &) const {
    SharedTasks subtasks;
    subtasks.reserve(num_copies);
    for (int i = 0; i < num_copies; ++i) {
        subtasks.push_back(task);
    }
    return subtasks;
}

GoalDecomposition::GoalDecomposition(const plugins::Options &opts)
    : fact_order(opts.get<FactOrder>("order")),
      rng(utils::parse_rng_from_options(opts)) {
}

SharedTasks GoalDecomposition::get_subtasks(
    const shared_ptr<AbstractTask> &task, utils::LogProxy &log) const {
    SharedTasks subtasks;
    TaskProxy task_proxy(*task);
    Facts goal_facts = task_properties::get_fact_pairs(task_proxy.get_goals());
    filter_and_order_facts(task, fact_order, goal_facts, *rng, log);
    for (const FactPair &goal : goal_facts) {
        shared_ptr<AbstractTask> subtask =
            make_shared<extra_tasks::ModifiedGoalsTask>(task, Facts {goal});
        subtasks.push_back(subtask);
    }
    return subtasks;
}


LandmarkDecomposition::LandmarkDecomposition(const plugins::Options &opts)
    : fact_order(opts.get<FactOrder>("order")),
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
    const shared_ptr<AbstractTask> &task, utils::LogProxy &log) const {
    SharedTasks subtasks;
    shared_ptr<landmarks::LandmarkGraph> landmark_graph =
        get_landmark_graph(task);
    Facts landmark_facts = get_fact_landmarks(*landmark_graph);
    filter_and_order_facts(task, fact_order, landmark_facts, *rng, log);
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

static void add_fact_order_option(plugins::Feature &feature) {
    feature.add_option<FactOrder>(
        "order",
        "ordering of goal or landmark facts",
        "hadd_down");
    utils::add_rng_options(feature);
}

class TaskDuplicatorFeature : public plugins::TypedFeature<SubtaskGenerator, TaskDuplicator> {
public:
    TaskDuplicatorFeature() : TypedFeature("original") {
        add_option<int>(
            "copies",
            "number of task copies",
            "1",
            plugins::Bounds("1", "infinity"));
    }
};

static plugins::FeaturePlugin<TaskDuplicatorFeature> _plugin_original;

class GoalDecompositionFeature : public plugins::TypedFeature<SubtaskGenerator, GoalDecomposition> {
public:
    GoalDecompositionFeature() : TypedFeature("goals") {
        add_fact_order_option(*this);
    }
};

static plugins::FeaturePlugin<GoalDecompositionFeature> _plugin_goals;


class LandmarkDecompositionFeature : public plugins::TypedFeature<SubtaskGenerator, LandmarkDecomposition> {
public:
    LandmarkDecompositionFeature() : TypedFeature("landmarks") {
        add_fact_order_option(*this);
        add_option<bool>(
            "combine_facts",
            "combine landmark facts with domain abstraction",
            "true");
    }
};

static plugins::FeaturePlugin<LandmarkDecompositionFeature> _plugin_landmarks;


static class SubtaskGeneratorCategoryPlugin : public plugins::TypedCategoryPlugin<SubtaskGenerator> {
public:
    SubtaskGeneratorCategoryPlugin() : TypedCategoryPlugin("SubtaskGenerator") {
        document_synopsis("Subtask generator (used by the CEGAR heuristic).");
    }
}
_category_plugin;

static plugins::TypedEnumPlugin<FactOrder> _enum_plugin({
        {"original", "according to their (internal) variable index"},
        {"random", "according to a random permutation"},
        {"hadd_up", "according to their h^add value, lowest first"},
        {"hadd_down", "according to their h^add value, highest first "}
    });
}
