#include "root_task.h"

#include "../state_registry.h"

#include "../plugins/plugin.h"
#include "../utils/collections.h"
#include "../utils/task_parser.h"
#include "../utils/timer.h"

#include <algorithm>
#include <cassert>
#include <memory>
#include <set>
#include <unordered_set>
#include <vector>


using namespace std;
using utils::ExitCode;

namespace tasks {
static const int PRE_FILE_VERSION = 3;
shared_ptr<AbstractTask> g_root_task = nullptr;

struct ExplicitVariable {
    int domain_size;
    string name;
    vector<string> fact_names;
    int axiom_layer;
    int axiom_default_value;

    explicit ExplicitVariable(utils::TaskParser &task_parser);
};


struct ExplicitEffect {
    FactPair fact;
    vector<FactPair> conditions;

    ExplicitEffect(int var, int value, vector<FactPair> &&conditions);
};


struct ExplicitOperator {
    vector<FactPair> preconditions;
    vector<ExplicitEffect> effects;
    int cost;
    string name;
    bool is_an_axiom;

    void read_pre_post(utils::TaskParser &task_parser);
    void read_axiom(utils::TaskParser &task_parser);
    ExplicitOperator(utils::TaskParser &task_parser, bool is_an_axiom, bool use_metric);
};


class RootTask : public AbstractTask {
    vector<ExplicitVariable> variables;
    // TODO: think about using hash sets here.
    vector<vector<set<FactPair>>> mutexes;
    vector<ExplicitOperator> operators;
    vector<ExplicitOperator> axioms;
    vector<int> initial_state_values;
    vector<FactPair> goals;

    const ExplicitVariable &get_variable(int var) const;
    const ExplicitEffect &get_effect(int op_id, int effect_id, bool is_axiom) const;
    const ExplicitOperator &get_operator_or_axiom(int index, bool is_axiom) const;

public:
    explicit RootTask(istream &in);

    virtual int get_num_variables() const override;
    virtual string get_variable_name(int var) const override;
    virtual int get_variable_domain_size(int var) const override;
    virtual int get_variable_axiom_layer(int var) const override;
    virtual int get_variable_default_axiom_value(int var) const override;
    virtual string get_fact_name(const FactPair &fact) const override;
    virtual bool are_facts_mutex(
        const FactPair &fact1, const FactPair &fact2) const override;

    virtual int get_operator_cost(int index, bool is_axiom) const override;
    virtual string get_operator_name(
        int index, bool is_axiom) const override;
    virtual int get_num_operators() const override;
    virtual int get_num_operator_preconditions(
        int index, bool is_axiom) const override;
    virtual FactPair get_operator_precondition(
        int op_index, int fact_index, bool is_axiom) const override;
    virtual int get_num_operator_effects(
        int op_index, bool is_axiom) const override;
    virtual int get_num_operator_effect_conditions(
        int op_index, int eff_index, bool is_axiom) const override;
    virtual FactPair get_operator_effect_condition(
        int op_index, int eff_index, int cond_index, bool is_axiom) const override;
    virtual FactPair get_operator_effect(
        int op_index, int eff_index, bool is_axiom) const override;
    virtual int convert_operator_index(
        int index, const AbstractTask *ancestor_task) const override;

    virtual int get_num_axioms() const override;

    virtual int get_num_goals() const override;
    virtual FactPair get_goal_fact(int index) const override;

    virtual vector<int> get_initial_state_values() const override;
    virtual void convert_ancestor_state_values(
        vector<int> &values,
        const AbstractTask *ancestor_task) const override;
};


static void check_fact(const FactPair &fact, const vector<ExplicitVariable> &variables) {
    if (!utils::in_bounds(fact.var, variables)) {
        cerr << "Invalid variable id: " << fact.var << endl;
        utils::exit_with(ExitCode::SEARCH_INPUT_ERROR);
    }
    if (fact.value < 0 || fact.value >= variables[fact.var].domain_size) {
        cerr << "Invalid value for variable " << fact.var << ": " << fact.value << endl;
        utils::exit_with(ExitCode::SEARCH_INPUT_ERROR);
    }
}

static void check_facts(const vector<FactPair> &facts, const vector<ExplicitVariable> &variables) {
    for (FactPair fact : facts) {
        check_fact(fact, variables);
    }
}

static void check_facts(const set<FactPair> &facts, const vector<ExplicitVariable> &variables) {
    for (FactPair fact : facts) {
        check_fact(fact, variables);
    }
}

static void check_facts(const ExplicitOperator &action, const vector<ExplicitVariable> &variables) {
    check_facts(action.preconditions, variables);
    for (const ExplicitEffect &eff : action.effects) {
        check_fact(eff.fact, variables);
        check_facts(eff.conditions, variables);
    }
}

static vector<FactPair> read_facts(utils::TaskParser &task_parser, bool read_from_single_line) {
    const string counter_name = "number of conditions";
    int count = read_from_single_line ? task_parser.read_int(counter_name)
                            : task_parser.read_line_int(counter_name);
    vector<FactPair> conditions;
    conditions.reserve(count);
    for (int i = 0; i < count; ++i) {
        FactPair condition = FactPair::no_fact;
        condition.var = task_parser.read_int("condition variable");
        condition.value = task_parser.read_int("condition value");
        if (!read_from_single_line) {
            task_parser.confirm_end_of_line();
        }
        conditions.push_back(condition);
    }
    return conditions;
}

ExplicitVariable::ExplicitVariable(utils::TaskParser &task_parser) {
    task_parser.read_magic_line("begin_variable");
    name = task_parser.read_line("variable name");
    axiom_layer = task_parser.read_line_int("variable axiom layer");
    domain_size = task_parser.read_line_int("variable domain size");
    if (domain_size < 1) {
        task_parser.error(
            "Domain size should be at least 1, but is "
            + std::to_string(domain_size) + ".");
    }
    fact_names.resize(domain_size);
    for (int i = 0; i < domain_size; ++i)
        fact_names[i] = task_parser.read_line("fact name");
    task_parser.read_magic_line("end_variable");
}


ExplicitEffect::ExplicitEffect(
    int var, int value, vector<FactPair> &&conditions)
    : fact(var, value), conditions(move(conditions)) {
}

void ExplicitOperator::read_pre_post(utils::TaskParser &task_parser) {
    vector<FactPair> conditions = read_facts(task_parser, true);
    int var = task_parser.read_int("variable affected by effect");
    int value_pre = task_parser.read_int("variable value precondition");
    int value_post = task_parser.read_int("variable value postcondition");
    if (value_pre != -1) {
        preconditions.emplace_back(var, value_pre);
    }
    task_parser.confirm_end_of_line();
    effects.emplace_back(var, value_post, move(conditions));
}

void ExplicitOperator::read_axiom(utils::TaskParser &task_parser) {
    vector<FactPair> conditions = read_facts(task_parser, false);
    int var = task_parser.read_int("variable affected by axiom");
    int value_pre = task_parser.read_int("variable value precondition");
    int value_post = task_parser.read_int("variable value postcondition");
    task_parser.confirm_end_of_line();
    if (value_pre != -1) {
        preconditions.emplace_back(var, value_pre);
    }
    effects.emplace_back(var, value_post, move(conditions));
}

ExplicitOperator::ExplicitOperator(utils::TaskParser &task_parser, bool is_an_axiom, bool use_metric)
    : is_an_axiom(is_an_axiom) {
    if (!is_an_axiom) {
        task_parser.read_magic_line("begin_operator");
        name = task_parser.read_line("operator name");
        preconditions = read_facts(task_parser, false);
        int count = task_parser.read_line_int("number of operator effects");
        effects.reserve(count);
        for (int i = 0; i < count; ++i) {
            read_pre_post(task_parser);
        }
        int op_cost = task_parser.read_line_int("operator cost");
        cost = use_metric ? op_cost : 1;
        task_parser.read_magic_line("end_operator");
    } else {
        name = "<axiom>";
        cost = 0;
        task_parser.read_magic_line("begin_rule");
        read_axiom(task_parser);
        task_parser.read_magic_line("end_rule");
    }
    assert(cost >= 0);
}

static void read_and_verify_version(utils::TaskParser &task_parser) {
    task_parser.set_context("version section");
    task_parser.read_magic_line("begin_version");
    int version = task_parser.read_line_int("version number");
    if (version != PRE_FILE_VERSION) {
        cerr << "Expected translator output file version " << PRE_FILE_VERSION
             << ", got " << version << "." << endl
             << "Exiting." << endl;
        utils::exit_with(ExitCode::SEARCH_INPUT_ERROR);
    }
    task_parser.read_magic_line("end_version");
}

static bool read_metric(utils::TaskParser &task_parser) {
    task_parser.set_context("metric section");
    task_parser.read_magic_line("begin_metric");
    string use_metric_string = task_parser.read_line("use metric");
    bool use_metric = false;
    if (use_metric_string == "1") {
        use_metric = true;
    } else if (use_metric_string == "0") {
        use_metric = false;
    } else {
        task_parser.error("expected 0 or 1");
    }
    task_parser.read_magic_line("end_metric");
    return use_metric;
}

static vector<ExplicitVariable> read_variables(utils::TaskParser &task_parser) {
    task_parser.set_context("variable section");
    int count = task_parser.read_line_int("variable count");
    if (count < 1) {
        task_parser.error(
            "Number of variables should be at least 1, but is "
            + std::to_string(count) + ".");
    }
    vector<ExplicitVariable> variables;
    variables.reserve(count);
    for (int i = 0; i < count; ++i) {
        variables.emplace_back(task_parser);
    }
    return variables;
}

static vector<vector<set<FactPair>>> read_mutexes(utils::TaskParser &task_parser, const vector<ExplicitVariable> &variables) {
    task_parser.set_context("mutex section");
    vector<vector<set<FactPair>>> inconsistent_facts(variables.size());
    for (size_t i = 0; i < variables.size(); ++i)
        inconsistent_facts[i].resize(variables[i].domain_size);

    int num_mutex_groups = task_parser.read_line_int("number of mutex groups");

    /*
      NOTE: Mutex groups can overlap, in which case the same mutex
      should not be represented multiple times. The current
      representation takes care of that automatically by using sets.
      If we ever change this representation, this is something to be
      aware of.
    */
    for (int i = 0; i < num_mutex_groups; ++i) {
        task_parser.read_magic_line("begin_mutex_group");
        int num_facts = task_parser.read_line_int("number of facts in mutex group");
        if (num_facts < 1) {
            task_parser.error(
                "Number of facts in mutex group should be at least 1, but is "
                + std::to_string(num_facts) + ".");
        }
        vector<FactPair> invariant_group;
        invariant_group.reserve(num_facts);
        for (int j = 0; j < num_facts; ++j) {
            int var = task_parser.read_int("variable number of mutex atom");
            int value = task_parser.read_int("value of mutex atom");
            task_parser.confirm_end_of_line();
            invariant_group.emplace_back(var, value);
        }
        task_parser.read_magic_line("end_mutex_group");
        set<FactPair> invariant_group_set(invariant_group.begin(), invariant_group.end());
        if (invariant_group_set.size() != invariant_group.size()) {
            task_parser.error("Mutex group may not contain the same fact more than once.");
        }
        for (const FactPair &fact1 : invariant_group) {
            for (const FactPair &fact2 : invariant_group) {
                if (fact1.var != fact2.var) {
                    /* The "different variable" test makes sure we
                       don't mark a fact as mutex with itself
                       (important for correctness) and don't include
                       redundant mutexes (important to conserve
                       memory). Note that the translator (at least
                       with default settings) removes mutex groups
                       that contain *only* redundant mutexes, but it
                       can of course generate mutex groups which lead
                       to *some* redundant mutexes, where some but not
                       all facts talk about the same variable. */
                    inconsistent_facts[fact1.var][fact1.value].insert(fact2);
                }
            }
        }
    }
    return inconsistent_facts;
}

static vector<FactPair> read_goal(utils::TaskParser &task_parser) {
    task_parser.set_context("goal section");
    task_parser.read_magic_line("begin_goal");
    vector<FactPair> goals = read_facts(task_parser, false);
    task_parser.read_magic_line("end_goal");
    if (goals.empty()) {
        cerr << "Task has no goal condition!" << endl;
        utils::exit_with(ExitCode::SEARCH_INPUT_ERROR);
    }
    return goals;
}

static vector<ExplicitOperator> read_actions(
    utils::TaskParser &task_parser, bool is_axiom, bool use_metric,
    const vector<ExplicitVariable> &variables) {
    task_parser.set_context(is_axiom ? "axiom section" : "operator section");
    int count = task_parser.read_line_int(is_axiom ? "number of axioms" : "number of operators");
    vector<ExplicitOperator> actions;
    actions.reserve(count);
    for (int i = 0; i < count; ++i) {
        actions.emplace_back(task_parser, is_axiom, use_metric);
        check_facts(actions.back(), variables);
    }
    return actions;
}

RootTask::RootTask(istream &in) {
    utils::TaskParser task_parser(in);
    read_and_verify_version(task_parser);
    bool use_metric = read_metric(task_parser);
    variables = read_variables(task_parser);
    int num_variables = variables.size();

    mutexes = read_mutexes(task_parser, variables);
    for (size_t i = 0; i < mutexes.size(); ++i) {
        for (size_t j = 0; j < mutexes[i].size(); ++j) {
            check_facts(mutexes[i][j], variables);
        }
    }

    // TODO: Maybe we could move this into a separate function as well
    task_parser.set_context("initial state section");
    initial_state_values.resize(num_variables);
    task_parser.read_magic_line("begin_state");
    for (int i = 0; i < num_variables; ++i) {
        initial_state_values[i] = task_parser.read_line_int("initial state variable value");
    }
    task_parser.read_magic_line("end_state");

    for (int i = 0; i < num_variables; ++i) {
        check_fact(FactPair(i, initial_state_values[i]), variables);
        variables[i].axiom_default_value = initial_state_values[i];
    }

    goals = read_goal(task_parser);
    check_facts(goals, variables);
    operators = read_actions(task_parser, false, use_metric, variables);
    axioms = read_actions(task_parser, true, use_metric, variables);
    task_parser.confirm_end_of_input();

    /*
      HACK: We use a TaskProxy to access g_axiom_evaluators here which assumes
      that this task is completely constructed.
    */
    AxiomEvaluator &axiom_evaluator = g_axiom_evaluators[TaskProxy(*this)];
    axiom_evaluator.evaluate(initial_state_values);
}

const ExplicitVariable &RootTask::get_variable(int var) const {
    assert(utils::in_bounds(var, variables));
    return variables[var];
}

const ExplicitEffect &RootTask::get_effect(
    int op_id, int effect_id, bool is_axiom) const {
    const ExplicitOperator &op = get_operator_or_axiom(op_id, is_axiom);
    assert(utils::in_bounds(effect_id, op.effects));
    return op.effects[effect_id];
}

const ExplicitOperator &RootTask::get_operator_or_axiom(
    int index, bool is_axiom) const {
    if (is_axiom) {
        assert(utils::in_bounds(index, axioms));
        return axioms[index];
    } else {
        assert(utils::in_bounds(index, operators));
        return operators[index];
    }
}

int RootTask::get_num_variables() const {
    return variables.size();
}

string RootTask::get_variable_name(int var) const {
    return get_variable(var).name;
}

int RootTask::get_variable_domain_size(int var) const {
    return get_variable(var).domain_size;
}

int RootTask::get_variable_axiom_layer(int var) const {
    return get_variable(var).axiom_layer;
}

int RootTask::get_variable_default_axiom_value(int var) const {
    return get_variable(var).axiom_default_value;
}

string RootTask::get_fact_name(const FactPair &fact) const {
    assert(utils::in_bounds(fact.value, get_variable(fact.var).fact_names));
    return get_variable(fact.var).fact_names[fact.value];
}

bool RootTask::are_facts_mutex(const FactPair &fact1, const FactPair &fact2) const {
    if (fact1.var == fact2.var) {
        // Same variable: mutex iff different value.
        return fact1.value != fact2.value;
    }
    assert(utils::in_bounds(fact1.var, mutexes));
    assert(utils::in_bounds(fact1.value, mutexes[fact1.var]));
    return bool(mutexes[fact1.var][fact1.value].count(fact2));
}

int RootTask::get_operator_cost(int index, bool is_axiom) const {
    return get_operator_or_axiom(index, is_axiom).cost;
}

string RootTask::get_operator_name(int index, bool is_axiom) const {
    return get_operator_or_axiom(index, is_axiom).name;
}

int RootTask::get_num_operators() const {
    return operators.size();
}

int RootTask::get_num_operator_preconditions(int index, bool is_axiom) const {
    return get_operator_or_axiom(index, is_axiom).preconditions.size();
}

FactPair RootTask::get_operator_precondition(
    int op_index, int fact_index, bool is_axiom) const {
    const ExplicitOperator &op = get_operator_or_axiom(op_index, is_axiom);
    assert(utils::in_bounds(fact_index, op.preconditions));
    return op.preconditions[fact_index];
}

int RootTask::get_num_operator_effects(int op_index, bool is_axiom) const {
    return get_operator_or_axiom(op_index, is_axiom).effects.size();
}

int RootTask::get_num_operator_effect_conditions(
    int op_index, int eff_index, bool is_axiom) const {
    return get_effect(op_index, eff_index, is_axiom).conditions.size();
}

FactPair RootTask::get_operator_effect_condition(
    int op_index, int eff_index, int cond_index, bool is_axiom) const {
    const ExplicitEffect &effect = get_effect(op_index, eff_index, is_axiom);
    assert(utils::in_bounds(cond_index, effect.conditions));
    return effect.conditions[cond_index];
}

FactPair RootTask::get_operator_effect(
    int op_index, int eff_index, bool is_axiom) const {
    return get_effect(op_index, eff_index, is_axiom).fact;
}

int RootTask::convert_operator_index(
    int index, const AbstractTask *ancestor_task) const {
    if (this != ancestor_task) {
        ABORT("Invalid operator ID conversion");
    }
    return index;
}

int RootTask::get_num_axioms() const {
    return axioms.size();
}

int RootTask::get_num_goals() const {
    return goals.size();
}

FactPair RootTask::get_goal_fact(int index) const {
    assert(utils::in_bounds(index, goals));
    return goals[index];
}

vector<int> RootTask::get_initial_state_values() const {
    return initial_state_values;
}

void RootTask::convert_ancestor_state_values(
    vector<int> &, const AbstractTask *ancestor_task) const {
    if (this != ancestor_task) {
        ABORT("Invalid state conversion");
    }
}

void read_root_task(istream &in) {
    assert(!g_root_task);
    g_root_task = make_shared<RootTask>(in);
}

class RootTaskFeature
    : public plugins::TypedFeature<AbstractTask, AbstractTask> {
public:
    RootTaskFeature() : TypedFeature("no_transform") {
    }

    virtual shared_ptr<AbstractTask> create_component(const plugins::Options &, const utils::Context &) const override {
        return g_root_task;
    }
};

static plugins::FeaturePlugin<RootTaskFeature> _plugin;
}
