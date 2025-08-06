#include "root_task.h"

#include "../axioms.h"

#include "../plugins/plugin.h"
#include "../utils/collections.h"
#include "../utils/task_lexer.h"

#include <cassert>
#include <memory>
#include <set>
#include <type_traits>
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
};

struct ExplicitEffect {
    FactPair fact;
    vector<FactPair> conditions;
};

struct ExplicitOperator {
    vector<FactPair> preconditions;
    vector<ExplicitEffect> effects;
    int cost;
    string name;
    bool is_an_axiom;
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
    const ExplicitEffect &get_effect(
        int op_id, int effect_id, bool is_axiom) const;
    const ExplicitOperator &get_operator_or_axiom(
        int index, bool is_axiom) const;

public:
    RootTask(
        vector<ExplicitVariable> &&variables,
        vector<vector<set<FactPair>>> &&mutexes,
        vector<ExplicitOperator> &&operators, vector<ExplicitOperator> &&axioms,
        vector<int> &&initial_state_values, vector<FactPair> &&goals);

    vector<int> &get_initial_state_values() {
        return initial_state_values;
    }

    virtual int get_num_variables() const override;
    virtual string get_variable_name(int var) const override;
    virtual int get_variable_domain_size(int var) const override;
    virtual int get_variable_axiom_layer(int var) const override;
    virtual int get_variable_default_axiom_value(int var) const override;
    virtual string get_fact_name(const FactPair &fact) const override;
    virtual bool are_facts_mutex(
        const FactPair &fact1, const FactPair &fact2) const override;

    virtual int get_operator_cost(int index, bool is_axiom) const override;
    virtual string get_operator_name(int index, bool is_axiom) const override;
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
        int op_index, int eff_index, int cond_index,
        bool is_axiom) const override;
    virtual FactPair get_operator_effect(
        int op_index, int eff_index, bool is_axiom) const override;
    virtual int convert_operator_index(
        int index, const AbstractTask *ancestor_task) const override;

    virtual int get_num_axioms() const override;

    virtual int get_num_goals() const override;
    virtual FactPair get_goal_fact(int index) const override;

    virtual vector<int> get_initial_state_values() const override;
    virtual void convert_ancestor_state_values(
        vector<int> &values, const AbstractTask *ancestor_task) const override;
};

class TaskParser {
    utils::TaskLexer lexer;

    template<typename Func, typename FuncOrStr>
    auto with_error_context(FuncOrStr &&message, Func &&func)
        -> invoke_result_t<Func> {
        int line = this->get_line_number();
        try {
            using ResultType = invoke_result_t<Func>;
            if constexpr (is_void_v<ResultType>) {
                forward<Func>(func)();
            } else {
                return forward<Func>(func)();
            }
        } catch (utils::TaskParserError &error) {
            if constexpr (is_convertible_v<FuncOrStr, string>) {
                this->add_error_line(error, line, message);
            } else {
                this->add_error_line(error, line, message());
            }
            throw;
        }
    }

    void error(const string &message) {
        throw utils::TaskParserError(message);
    }

    void add_error_line(
        utils::TaskParserError &error, int line_no, const string &line) const {
        error.add_context("[line " + to_string(line_no) + "] " + line);
    }

    int get_line_number() const {
        return lexer.get_line_number();
    }

    void check_fact(
        const FactPair &fact, const vector<ExplicitVariable> &variables);
    void check_layering_condition(
        int head_var_layer, const vector<FactPair> &conditions,
        const vector<ExplicitVariable> &variables);

    int parse_int(const string &token);
    void check_nat(const string &value_name, int value);
    int read_int(const string &value_name);
    int read_nat(const string &value_name);
    string read_string_line(const string &value_name);
    void read_magic_line(const string &magic);

    void read_and_verify_version();
    bool read_metric();
    vector<ExplicitVariable> read_variables();
    ExplicitVariable read_variable(int index);
    vector<FactPair> read_facts(
        bool read_from_single_line, const vector<ExplicitVariable> &variables);
    vector<vector<set<FactPair>>> read_mutexes(
        const vector<ExplicitVariable> &variables);
    vector<int> read_initial_state(const vector<ExplicitVariable> &variables);
    vector<FactPair> read_goal(const vector<ExplicitVariable> &variables);
    vector<ExplicitOperator> read_actions(
        bool is_axiom, bool use_metric,
        const vector<ExplicitVariable> &variables);
    ExplicitOperator read_operator(
        int index, bool use_metric, const vector<ExplicitVariable> &variables);
    ExplicitOperator read_axiom(
        int index, const vector<ExplicitVariable> &variables);
    void read_conditional_effect(
        ExplicitOperator &op, const vector<ExplicitVariable> &variables);
    void read_pre_post_axiom(
        ExplicitOperator &op, const vector<ExplicitVariable> &variables);
    shared_ptr<AbstractTask> read_task();

public:
    TaskParser(utils::TaskLexer &&lexer);
    shared_ptr<AbstractTask> parse();
};

void TaskParser::check_fact(
    const FactPair &fact, const vector<ExplicitVariable> &variables) {
    if (!utils::in_bounds(fact.var, variables)) {
        error("Invalid variable id: " + to_string(fact.var));
    } else if (
        fact.value < 0 || fact.value >= variables[fact.var].domain_size) {
        error(
            "Invalid value for variable '" + variables[fact.var].name +
            "' (index: " + to_string(fact.var) + "): " + to_string(fact.value));
    }
}

void TaskParser::check_layering_condition(
    int head_var_layer, const vector<FactPair> &conditions,
    const vector<ExplicitVariable> &variables) {
    for (FactPair fact : conditions) {
        int var = fact.var;
        int var_layer = variables[var].axiom_layer;
        if (var_layer > head_var_layer) {
            error(
                "Variables must be at head variable layer or below, but variable " +
                to_string(var) + " is at layer " + to_string(var_layer) + ".");
        }
        if (var_layer == head_var_layer) {
            int default_value = variables[var].axiom_default_value;
            if (fact.value == default_value) {
                error(
                    "Body variables at head variable layer must "
                    "have non-default value, but variable " +
                    to_string(var) + " uses default value " +
                    to_string(fact.value) + ".");
            }
        }
    }
}

int TaskParser::parse_int(const string &token) {
    string failure_reason;
    int number = 0;
    try {
        string::size_type parsed_length;
        number = stoi(token, &parsed_length);
        if (parsed_length != token.size()) {
            failure_reason =
                "invalid character '" + string(1, token[parsed_length]) + "'";
        }
    } catch (invalid_argument &) {
        failure_reason = "invalid argument";
    } catch (out_of_range &) {
        failure_reason = "out of range";
    }
    if (!failure_reason.empty()) {
        error(
            "Could not parse '" + token + "' as integer (" + failure_reason +
            ").");
    }
    return number;
}

// TODO: rename to check_natural_number
void TaskParser::check_nat(const string &value_name, int value) {
    if (value < 0) {
        error(
            "Expected non-negative number for " + value_name + ", but got " +
            to_string(value) + ".");
    }
}

int TaskParser::read_int(const string &value_name) {
    return with_error_context(
        value_name, [&]() { return parse_int(lexer.read()); });
}

// TODO: rename to read_natural_number
int TaskParser::read_nat(const string &value_name) {
    int value = read_int(value_name);
    check_nat(value_name, value);
    return value;
}

string TaskParser::read_string_line(const string &value_name) {
    return with_error_context(value_name, [&]() { return lexer.read_line(); });
}

void TaskParser::read_magic_line(const string &magic) {
    return with_error_context("magic value", [&]() {
        string line = read_string_line("magic value");
        if (line != magic) {
            error("Expected magic line '" + magic + "', got '" + line + "'.");
        }
    });
}

// TODO: consistent naming (facts vs. conditions)
vector<FactPair> TaskParser::read_facts(
    bool read_from_single_line, const vector<ExplicitVariable> &variables) {
    return with_error_context("parsing conditions", [&]() {
        string value_name = "number of conditions";
        int count = read_nat(value_name);
        if (!read_from_single_line) {
            lexer.confirm_end_of_line();
        }
        vector<FactPair> conditions;
        conditions.reserve(count);
        for (int i = 0; i < count; ++i) {
            with_error_context(
                [&]() { return "fact" + to_string(i); },
                [&]() {
                    FactPair condition = FactPair::no_fact;
                    condition.var = read_nat("variable");
                    condition.value = read_nat("value");
                    check_fact(condition, variables);
                    conditions.push_back(condition);
                    if (!read_from_single_line) {
                        lexer.confirm_end_of_line();
                    }
                });
        }
        return conditions;
    });
}

ExplicitVariable TaskParser::read_variable(int index) {
    ExplicitVariable var;
    with_error_context(
        [&]() { return "parsing variable " + to_string(index); },
        [&]() {
            read_magic_line("begin_variable");
            var.name = read_string_line("variable name");
        });
    /*
      We close the previous block and open a new one, now that we have the
      name of the variable. This produces better error messages but the line
      number for the second block will be the one after the name.
    */
    return with_error_context(
        [&]() { return "parsing definition of variable '" + var.name + "'"; },
        [&]() {
            var.axiom_layer = read_int("variable axiom layer");
            if (var.axiom_layer < -1) {
                error(
                    "Variable axiom layer must be -1 or non-negative, but is " +
                    to_string(var.axiom_layer) + ".");
            }
            lexer.confirm_end_of_line();
            var.domain_size = read_nat("variable domain size");
            if (var.domain_size < 1) {
                error(
                    "Domain size should be at least 1, but is " +
                    to_string(var.domain_size) + ".");
            }
            if ((var.axiom_layer >= 0) && var.domain_size != 2) {
                error(
                    "Derived variables must be binary, but domain size is " +
                    to_string(var.domain_size) + ".");
            }
            lexer.confirm_end_of_line();
            var.fact_names.resize(var.domain_size);
            for (int i = 0; i < var.domain_size; ++i) {
                with_error_context(
                    [&]() { return "fact " + to_string(i); },
                    [&]() { var.fact_names[i] = read_string_line("name"); });
            }
            read_magic_line("end_variable");
            return var;
        });
}

void TaskParser::read_pre_post_axiom(
    ExplicitOperator &op, const vector<ExplicitVariable> &variables) {
    vector<FactPair> conditions = read_facts(false, variables);
    with_error_context("parsing pre-post of affected variable", [&]() {
        int var = read_nat("affected variable");
        int axiom_layer = variables[var].axiom_layer;
        if (axiom_layer == -1) {
            error(
                "Variable affected by axiom must be derived, but variable " +
                to_string(var) + " is not derived.");
        }
        int value_pre = read_int("variable value precondition");
        if (value_pre == -1) {
            error(
                "Variable affected by axiom must have precondition, but value is -1.");
        }
        FactPair precondition = FactPair(var, value_pre);
        check_fact(precondition, variables);
        int value_post = read_nat("variable value postcondition");
        FactPair postcondition = FactPair(var, value_post);
        check_fact(postcondition, variables);
        int default_value = variables[var].axiom_default_value;
        assert(default_value != -1);
        if (value_pre != default_value) {
            error(
                "Value of variable affected by axiom must be default value " +
                to_string(default_value) + " in precondition, but is " +
                to_string(value_pre) + ".");
        }
        if (value_post == default_value) {
            error(
                "Value of variable affected by axiom must be non-default "
                "value in postcondition, but is default value " +
                to_string(value_post) + ".");
        }
        with_error_context(
            [&]() {
                return "checking layering condition, head variable " +
                       to_string(var) + " with layer " + to_string(axiom_layer);
            },
            [&]() {
                check_layering_condition(axiom_layer, conditions, variables);
            });
        op.preconditions.emplace_back(precondition);
        ExplicitEffect eff = {postcondition, move(conditions)};
        op.effects.emplace_back(eff);
        lexer.confirm_end_of_line();
    });
}

void TaskParser::read_conditional_effect(
    ExplicitOperator &op, const vector<ExplicitVariable> &variables) {
    vector<FactPair> conditions = read_facts(true, variables);
    with_error_context("parsing pre-post of affected variable", [&]() {
        int var = read_nat("affected variable");
        int axiom_layer = variables[var].axiom_layer;
        if (axiom_layer != -1) {
            error(
                "Variable affected by operator must not be derived, but variable " +
                to_string(var) + " is derived.");
        }
        int value_pre = read_int("variable value precondition");
        if (value_pre < -1) {
            error(
                "Variable value precondition must be -1 or non-negative, but is " +
                to_string(value_pre) + ".");
        }
        if (value_pre != -1) {
            FactPair precondition = FactPair(var, value_pre);
            check_fact(precondition, variables);
            op.preconditions.emplace_back(precondition);
        }
        int value_post = read_nat("variable value postcondition");
        FactPair postcondition = FactPair(var, value_post);
        check_fact(postcondition, variables);
        ExplicitEffect eff = {postcondition, move(conditions)};
        op.effects.emplace_back(eff);
        lexer.confirm_end_of_line();
    });
}

ExplicitOperator TaskParser::read_operator(
    int index, bool use_metric, const vector<ExplicitVariable> &variables) {
    ExplicitOperator op;
    op.is_an_axiom = false;
    with_error_context(
        [&]() { return "operator " + to_string(index); },
        [&]() {
            read_magic_line("begin_operator");
            op.name = read_string_line("operator name");
        });
    /*
      We close the previous block and open a new one, now that we have the
      name of the operator. This produces better error messages but the line
      number for the second block will be the one after the name.
    */
    return with_error_context(
        [&]() { return "parsing definition of operator '" + op.name + "'"; },
        [&]() {
            with_error_context("parsing prevail conditions", [&]() {
                op.preconditions = read_facts(false, variables);
            });
            int count = read_nat("number of operator effects");
            if (count < 1) {
                error(
                    "Number of operator effects should be at least 1, but is " +
                    to_string(count) + ".");
            }
            lexer.confirm_end_of_line();
            op.effects.reserve(count);
            for (int i = 0; i < count; ++i) {
                with_error_context(
                    [&]() { return "parsing effect " + to_string(i); },
                    [&]() { read_conditional_effect(op, variables); });
            }
            with_error_context("parsing operator cost", [&]() {
                int specified_cost = read_int("cost");
                op.cost = use_metric ? specified_cost : 1;
                if (op.cost < 0) {
                    error(
                        "Operator cost must be non-negative, but is " +
                        to_string(op.cost) + ".");
                }
                lexer.confirm_end_of_line();
            });
            read_magic_line("end_operator");
            return op;
        });
}

ExplicitOperator TaskParser::read_axiom(
    int index, const vector<ExplicitVariable> &variables) {
    return with_error_context(
        [&]() { return "axiom " + to_string(index); },
        [&]() {
            ExplicitOperator op;
            op.is_an_axiom = true;
            op.name = "<axiom>";
            op.cost = 0;

            read_magic_line("begin_rule");
            read_pre_post_axiom(op, variables);
            read_magic_line("end_rule");
            return op;
        });
}

void TaskParser::read_and_verify_version() {
    with_error_context("version section", [&]() {
        read_magic_line("begin_version");
        int version = read_nat("version number");
        if (version != PRE_FILE_VERSION) {
            error(
                "Expected translator output file version " +
                to_string(PRE_FILE_VERSION) + ", got " + to_string(version) +
                ".");
        }
        lexer.confirm_end_of_line();
        read_magic_line("end_version");
    });
}

bool TaskParser::read_metric() {
    return with_error_context("metric section", [&]() {
        read_magic_line("begin_metric");
        int use_metric_int = read_int("metric value");
        bool use_metric = false;
        if (use_metric_int == 1) {
            use_metric = true;
        } else if (use_metric_int != 0) {
            error("expected 0 or 1, got '" + to_string(use_metric_int) + "'.");
        }
        lexer.confirm_end_of_line();
        read_magic_line("end_metric");
        return use_metric;
    });
}

vector<ExplicitVariable> TaskParser::read_variables() {
    return with_error_context("variable section", [&]() {
        int count = read_nat("variable count");
        if (count < 1) {
            error(
                "Number of variables should be at least 1, but is " +
                to_string(count) + ".");
        }
        lexer.confirm_end_of_line();
        vector<ExplicitVariable> variables;
        variables.reserve(count);
        for (int i = 0; i < count; ++i) {
            variables.push_back(read_variable(i));
        }
        return variables;
    });
}

vector<vector<set<FactPair>>> TaskParser::read_mutexes(
    const vector<ExplicitVariable> &variables) {
    return with_error_context("mutex section", [&]() {
        vector<vector<set<FactPair>>> inconsistent_facts(variables.size());
        for (size_t i = 0; i < variables.size(); ++i)
            inconsistent_facts[i].resize(variables[i].domain_size);

        int num_mutex_groups = read_nat("number of mutex groups");
        lexer.confirm_end_of_line();

        /*
          NOTE: Mutex groups can overlap, in which case the same mutex
          should not be represented multiple times. The current
          representation takes care of that automatically by using sets.
          If we ever change this representation, this is something to be
          aware of.
        */
        for (int i = 0; i < num_mutex_groups; ++i) {
            with_error_context(
                [&]() { return "mutex group " + to_string(i); },
                [&]() {
                    read_magic_line("begin_mutex_group");
                    /* TODO: we should use read_facts here, but we need to
                       reconsider the error messages.
                    */
                    int num_facts = read_nat("number of facts in mutex group");
                    lexer.confirm_end_of_line();
                    vector<FactPair> invariant_group;
                    invariant_group.reserve(num_facts);
                    for (int j = 0; j < num_facts; ++j) {
                        with_error_context(
                            [&]() { return "mutex atom " + to_string(j); },
                            [&]() {
                                int var =
                                    read_nat("variable number of mutex atom");
                                int value = read_nat("value of mutex atom");
                                FactPair fact = FactPair(var, value);
                                check_fact(fact, variables);
                                invariant_group.emplace_back(fact);
                                lexer.confirm_end_of_line();
                            });
                    }
                    read_magic_line("end_mutex_group");

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
                                   can of course generate mutex groups which
                                   lead to *some* redundant mutexes, where some
                                   but not all facts talk about the same
                                   variable. */
                                inconsistent_facts[fact1.var][fact1.value]
                                    .insert(fact2);
                            }
                        }
                    }
                });
        }
        return inconsistent_facts;
    });
}

vector<int> TaskParser::read_initial_state(
    const vector<ExplicitVariable> &variables) {
    return with_error_context("initial state section", [&]() {
        read_magic_line("begin_state");
        int num_variables = variables.size();
        vector<int> initial_state_values(num_variables);
        for (int i = 0; i < num_variables; ++i) {
            string block_name = "initial state value of variable '" +
                                variables[i].name +
                                "' (index: " + to_string(i) + ")";
            with_error_context(
                [&]() { return "validating " + block_name; },
                [&]() {
                    initial_state_values[i] = read_nat(block_name);
                    lexer.confirm_end_of_line();
                    check_fact(FactPair(i, initial_state_values[i]), variables);
                });
        }
        read_magic_line("end_state");
        return initial_state_values;
    });
}

vector<FactPair> TaskParser::read_goal(
    const vector<ExplicitVariable> &variables) {
    return with_error_context("goal section", [&]() {
        read_magic_line("begin_goal");
        vector<FactPair> goals = read_facts(false, variables);
        read_magic_line("end_goal");
        // TODO: in the future, we would like to allow empty goals (issue 1160)
        if (goals.empty()) {
            cerr << "Task has no goal condition!" << endl;
            utils::exit_with(ExitCode::SEARCH_INPUT_ERROR);
        }
        // TODO: in the future, we would like to allow trivially unsolvable
        // tasks (issue 1160)
        set<int> goal_vars;
        for (FactPair goal : goals) {
            int var = goal.var;
            if (!goal_vars.insert(var).second) {
                error(
                    "Goal variables must be unique, but variable " +
                    to_string(var) + " occurs several times.");
            }
        }
        return goals;
    });
}

vector<ExplicitOperator> TaskParser::read_actions(
    bool is_axiom, bool use_metric, const vector<ExplicitVariable> &variables) {
    return with_error_context(
        [&]() { return is_axiom ? "axiom section" : "operator section"; },
        [&]() {
            int count = read_nat("number of entries");
            lexer.confirm_end_of_line();
            vector<ExplicitOperator> actions;
            actions.reserve(count);
            for (int i = 0; i < count; ++i) {
                ExplicitOperator action =
                    is_axiom ? read_axiom(i, variables)
                             : read_operator(i, use_metric, variables);
                actions.push_back(action);
            }
            return actions;
        });
}

TaskParser::TaskParser(utils::TaskLexer &&lexer) : lexer(move(lexer)) {
}

shared_ptr<AbstractTask> TaskParser::read_task() {
    read_and_verify_version();
    bool use_metric = read_metric();
    vector<ExplicitVariable> variables = read_variables();
    vector<vector<set<FactPair>>> mutexes = read_mutexes(variables);
    vector<int> initial_state_values = read_initial_state(variables);
    int num_variables = variables.size();
    for (int i = 0; i < num_variables; ++i) {
        variables[i].axiom_default_value = initial_state_values[i];
    }
    vector<FactPair> goals = read_goal(variables);
    vector<ExplicitOperator> operators =
        read_actions(false, use_metric, variables);
    vector<ExplicitOperator> axioms = read_actions(true, use_metric, variables);
    with_error_context(
        "confirm end of input", [&]() { lexer.confirm_end_of_input(); });

    /*
      "Neat Trick" and certainly no HACK:
      We currently require initial_state_values to contain values for derived
      variables. To evaluate the axioms, we require a task. But since evaluating
      axioms does not depend on the initial state, we can construct the task
      with a non-extended initial state (i.e. one without axioms evaluated) and
      then evaluate the axioms on that state after construction. This requires
      mutable access to the values, though.
    */
    shared_ptr<RootTask> task = make_shared<RootTask>(
        move(variables), move(mutexes), move(operators), move(axioms),
        move(initial_state_values), move(goals));
    AxiomEvaluator &axiom_evaluator = g_axiom_evaluators[TaskProxy(*task)];
    axiom_evaluator.evaluate(task->get_initial_state_values());
    return task;
}

shared_ptr<AbstractTask> TaskParser::parse() {
    try {
        return read_task();
    } catch (const utils::TaskParserError &error) {
        cerr << "Error reading task" << endl;
        error.print_with_context(cerr);
        utils::exit_with(ExitCode::SEARCH_INPUT_ERROR);
    }
}

RootTask::RootTask(
    vector<ExplicitVariable> &&variables,
    vector<vector<set<FactPair>>> &&mutexes,
    vector<ExplicitOperator> &&operators, vector<ExplicitOperator> &&axioms,
    vector<int> &&initial_state_values, vector<FactPair> &&goals)
    : variables(move(variables)),
      mutexes(move(mutexes)),
      operators(move(operators)),
      axioms(move(axioms)),
      initial_state_values(move(initial_state_values)),
      goals(move(goals)) {
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

bool RootTask::are_facts_mutex(
    const FactPair &fact1, const FactPair &fact2) const {
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
    utils::TaskLexer lexer(in);
    // TODO: construct lexer in TaskParser
    TaskParser parser(move(lexer));
    g_root_task = parser.parse();
}

class RootTaskFeature
    : public plugins::TypedFeature<AbstractTask, AbstractTask> {
public:
    RootTaskFeature() : TypedFeature("no_transform") {
        document_title("Root task");
        document_synopsis("No transformation of the input task.");
    }

    virtual shared_ptr<AbstractTask> create_component(
        const plugins::Options &) const override {
        return g_root_task;
    }
};

static plugins::FeaturePlugin<RootTaskFeature> _plugin;
}
