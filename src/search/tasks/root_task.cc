#include "root_task.h"

#include "../global_operator.h"
#include "../globals.h"
#include "../option_parser.h"
#include "../plugin.h"
#include "../state_registry.h"

#include "../utils/collections.h"
#include "../utils/timer.h"

#include <algorithm>
#include <cassert>
#include <memory>
#include <unordered_set>

using namespace std;
using utils::ExitCode;

namespace tasks {
static const int PRE_FILE_VERSION = 3;

void read_and_verify_version(istream &in) {
    int version;
    check_magic(in, "begin_version");
    in >> version;
    check_magic(in, "end_version");
    if (version != PRE_FILE_VERSION) {
        cerr << "Expected preprocessor file version " << PRE_FILE_VERSION
             << ", got " << version << "." << endl;
        cerr << "Exiting." << endl;
        utils::exit_with(ExitCode::INPUT_ERROR);
    }
}

void read_metric(istream &in) {
    check_magic(in, "begin_metric");
    in >> g_use_metric;
    check_magic(in, "end_metric");
}

void read_variables(istream &in) {
    int count;
    in >> count;
    for (int i = 0; i < count; ++i) {
        check_magic(in, "begin_variable");
        string name;
        in >> name;
        g_variable_name.push_back(name);
        int layer;
        in >> layer;
        g_axiom_layers.push_back(layer);
        int range;
        in >> range;
        g_variable_domain.push_back(range);
        in >> ws;
        vector<string> fact_names(range);
        for (size_t j = 0; j < fact_names.size(); ++j)
            getline(in, fact_names[j]);
        g_fact_names.push_back(fact_names);
        check_magic(in, "end_variable");
    }
}

void read_mutexes(istream &in) {
    g_inconsistent_facts.resize(g_variable_domain.size());
    for (size_t i = 0; i < g_variable_domain.size(); ++i)
        g_inconsistent_facts[i].resize(g_variable_domain[i]);

    int num_mutex_groups;
    in >> num_mutex_groups;

    /* NOTE: Mutex groups can overlap, in which case the same mutex
       should not be represented multiple times. The current
       representation takes care of that automatically by using sets.
       If we ever change this representation, this is something to be
       aware of. */

    for (int i = 0; i < num_mutex_groups; ++i) {
        check_magic(in, "begin_mutex_group");
        int num_facts;
        in >> num_facts;
        vector<FactPair> invariant_group;
        invariant_group.reserve(num_facts);
        for (int j = 0; j < num_facts; ++j) {
            int var;
            int value;
            in >> var >> value;
            invariant_group.emplace_back(var, value);
        }
        check_magic(in, "end_mutex_group");
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
                    g_inconsistent_facts[fact1.var][fact1.value].insert(fact2);
                }
            }
        }
    }
}

void read_goal(istream &in) {
    check_magic(in, "begin_goal");
    int count;
    in >> count;
    if (count < 1) {
        cerr << "Task has no goal condition!" << endl;
        utils::exit_with(ExitCode::INPUT_ERROR);
    }
    for (int i = 0; i < count; ++i) {
        int var, val;
        in >> var >> val;
        g_goal.push_back(make_pair(var, val));
    }
    check_magic(in, "end_goal");
}

void read_operators(istream &in) {
    int count;
    in >> count;
    for (int i = 0; i < count; ++i)
        g_operators.push_back(GlobalOperator(in, false));
}

void read_axioms(istream &in) {
    int count;
    in >> count;
    for (int i = 0; i < count; ++i)
        g_axioms.push_back(GlobalOperator(in, true));

    g_axiom_evaluator = new AxiomEvaluator(TaskProxy(*g_root_task));
}

std::shared_ptr<RootTask> parse_root_task(std::istream &in) {
    cout << "reading input... [t=" << utils::g_timer << "]" << endl;
    read_and_verify_version(in);
    read_metric(in);
    read_variables(in);
    read_mutexes(in);
    g_initial_state_data.resize(g_variable_domain.size());
    check_magic(in, "begin_state");
    for (size_t i = 0; i < g_variable_domain.size(); ++i) {
        in >> g_initial_state_data[i];
    }
    check_magic(in, "end_state");
    g_default_axiom_values = g_initial_state_data;

    read_goal(in);
    read_operators(in);
    read_axioms(in);

    /* TODO: We should be stricter here and verify that we
       have reached the end of "in". */

    cout << "done reading input! [t=" << utils::g_timer << "]" << endl;

    cout << "packing state variables..." << flush;
    assert(!g_variable_domain.empty());
    g_state_packer = new int_packer::IntPacker(g_variable_domain);
    cout << "done! [t=" << utils::g_timer << "]" << endl;

    int num_vars = g_variable_domain.size();
    int num_facts = 0;
    for (int var = 0; var < num_vars; ++var)
        num_facts += g_variable_domain[var];

    cout << "Variables: " << num_vars << endl;
    cout << "FactPairs: " << num_facts << endl;
    cout << "Bytes per state: "
         << g_state_packer->get_num_bins() * sizeof(int_packer::IntPacker::Bin)
         << endl;

    return create_root_task(
        g_variable_name, g_variable_domain, g_fact_names, g_axiom_layers,
        g_default_axiom_values, g_inconsistent_facts, g_initial_state_data,
        g_goal, g_operators, g_axioms);
}

#ifndef NDEBUG
static bool check_fact(const FactPair &fact) {
    /*
      We don't put this check into the FactPair ctor to allow (-1, -1)
      facts. Here, we want to make sure that the fact is valid.
    */
    return fact.var >= 0 && fact.value >= 0;
}
#endif

RootTask::RootTask(
    vector<ExplicitVariable> &&variables,
    vector<vector<set<FactPair>>> &&mutexes,
    vector<ExplicitOperator> &&operators,
    vector<ExplicitOperator> &&axioms,
    vector<int> &&initial_state_values,
    vector<FactPair> &&goals)
    : variables(move(variables)),
      mutexes(move(mutexes)),
      operators(move(operators)),
      axioms(move(axioms)),
      initial_state_values(move(initial_state_values)),
      goals(move(goals)),
      evaluated_axioms_on_initial_state(false) {
    assert(run_sanity_check());
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

bool RootTask::run_sanity_check() const {
    assert(initial_state_values.size() == variables.size());

    function<bool(const ExplicitOperator &op)> is_axiom =
        [](const ExplicitOperator &op) {
            return op.is_an_axiom;
        };
    assert(none_of(operators.begin(), operators.end(), is_axiom));
    assert(all_of(axioms.begin(), axioms.end(), is_axiom));

    // Check that each variable occurs at most once in the goal.
    unordered_set<int> goal_vars;
    for (const FactPair &goal: goals) {
        goal_vars.insert(goal.var);
    }
    assert(goal_vars.size() == goals.size());
    return true;
}

void RootTask::evaluate_axioms_on_initial_state() const {
    if (!axioms.empty()) {
        // HACK this should not have to go through a state registry.
        // HACK on top of the HACK above: this should not use globals.
        StateRegistry state_registry(
            *this, *g_state_packer, *g_axiom_evaluator, initial_state_values);
        initial_state_values = state_registry.get_initial_state().get_values();
    }
    evaluated_axioms_on_initial_state = true;
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

OperatorID RootTask::get_global_operator_id(OperatorID id) const {
    return id;
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
    if (!evaluated_axioms_on_initial_state) {
        evaluate_axioms_on_initial_state();
    }
    return initial_state_values;
}

void RootTask::convert_state_values(
    vector<int> &, const AbstractTask *ancestor_task) const {
    if (this != ancestor_task) {
        ABORT("Invalid state conversion");
    }
}


ExplicitVariable::ExplicitVariable(
    int domain_size,
    string &&name,
    vector<string> &&fact_names,
    int axiom_layer,
    int axiom_default_value)
    : domain_size(domain_size),
      name(move(name)),
      fact_names(move(fact_names)),
      axiom_layer(axiom_layer),
      axiom_default_value(axiom_default_value) {
    assert(domain_size >= 1);
    assert(static_cast<int>(this->fact_names.size()) == domain_size);
    assert(axiom_layer >= -1);
    assert(axiom_default_value >= -1);
}


ExplicitEffect::ExplicitEffect(
    int var, int value, vector<FactPair> &&conditions)
    : fact(var, value), conditions(move(conditions)) {
    assert(check_fact(FactPair(var, value)));
    assert(all_of(
               this->conditions.begin(), this->conditions.end(), check_fact));
}


ExplicitOperator::ExplicitOperator(
    vector<FactPair> &&preconditions,
    vector<ExplicitEffect> &&effects,
    int cost,
    const string &name,
    bool is_an_axiom)
    : preconditions(move(preconditions)),
      effects(move(effects)),
      cost(cost),
      name(name),
      is_an_axiom(is_an_axiom) {
    assert(all_of(
               this->preconditions.begin(),
               this->preconditions.end(),
               check_fact));
    assert(cost >= 0);
}

ExplicitOperator explicit_operator_from_global_operator(const GlobalOperator &op, bool is_axiom) {
    vector<FactPair> preconditions;
    preconditions.reserve(op.get_preconditions().size());
    for (const GlobalCondition &condition : op.get_preconditions()) {
        preconditions.emplace_back(condition.var, condition.val);
    }

    vector<ExplicitEffect> effects;
    effects.reserve(op.get_effects().size());
    for (const GlobalEffect &eff : op.get_effects()) {
        vector<FactPair> eff_conditions;
        eff_conditions.reserve(eff.conditions.size());
        for (const GlobalCondition &condition : eff.conditions) {
            eff_conditions.emplace_back(condition.var, condition.val);
        }
        effects.emplace_back(eff.var, eff.val, move(eff_conditions));
    }

    return ExplicitOperator(
        move(preconditions),
        move(effects),
        op.get_cost(),
        op.get_name(),
        is_axiom);
}

shared_ptr<RootTask> create_root_task(
    const vector<string> &variable_name,
    const vector<int> &variable_domain,
    const vector<vector<string>> &fact_names,
    const vector<int> &axiom_layers,
    const vector<int> &default_axiom_values,
    const vector<vector<set<FactPair>>> &inconsistent_facts,
    const vector<int> &initial_state_data,
    const vector<pair<int, int>> &goal,
    const vector<GlobalOperator> &global_operators,
    const vector<GlobalOperator> &global_axioms) {
    vector<ExplicitVariable> variables;
    variables.reserve(variable_domain.size());
    for (int i = 0; i < static_cast<int>(variable_domain.size()); ++i) {
        string name = variable_name[i];
        vector<string> var_fact_names = fact_names[i];
        variables.emplace_back(
            variable_domain[i],
            move(name),
            move(var_fact_names),
            axiom_layers[i],
            default_axiom_values[i]);
    }

    vector<vector<set<FactPair>>> mutexes = inconsistent_facts;


    vector<ExplicitOperator> operators;
    operators.reserve(operators.size());
    for (const GlobalOperator &op : global_operators) {
        operators.push_back(explicit_operator_from_global_operator(op, false));
    }

    vector<ExplicitOperator> axioms;
    axioms.reserve(axioms.size());
    for (const GlobalOperator &axiom : global_axioms) {
        axioms.push_back(explicit_operator_from_global_operator(axiom, true));
    }

    vector<int> initial_state_values = initial_state_data;

    vector<FactPair> goals;
    goals.reserve(goal.size());
    for (pair<int, int> g : goal) {
        goals.emplace_back(g.first, g.second);
    }

    return make_shared<RootTask>(
        move(variables),
        move(mutexes),
        move(operators),
        move(axioms),
        move(initial_state_values),
        move(goals));
}

static shared_ptr<AbstractTask> _parse(OptionParser &parser) {
    if (parser.dry_run())
        return nullptr;
    else
        return g_root_task;
}

static PluginShared<AbstractTask> _plugin("no_transform", _parse);
}
