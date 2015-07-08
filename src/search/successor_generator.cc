#include "successor_generator.h"

#include "task_tools.h"

#include <algorithm>
#include <cassert>
#include <cstdlib>
#include <fstream>
#include <functional>
#include <iostream>
#include <vector>
using namespace std;

/* NOTE on possible optimizations:

   * Sharing "GeneratorEmpty" instances might help quite a bit with
     reducing memory usage and possibly even speed, because there are
     bound to be many instance of those. However, it complicates
     deleting the successor generator, and memory doesn't seem to be
     an issue. Speed appears to be fine now, too. So this is probably
     an unnecessary complication.

   * Using slist instead of list led to a further 10% speedup on the
     largest Logistics instance, logistics-98/prob28.pddl. It would of
     course also reduce memory usage. However, it would make the code
     g++-specific, so it's probably not worth it.

*/

class GeneratorBase {
public:
    virtual ~GeneratorBase() = default;
    virtual void dump(string indent) const = 0;
    virtual void generate_cpp_input(ofstream &outfile) const = 0;
};

class GeneratorSwitch : public GeneratorBase {
    VariableProxy switch_var;
    list<OperatorProxy> immediate_operators;
    vector<GeneratorBase *> generator_for_value;
    GeneratorBase *default_generator;
public:
    ~GeneratorSwitch();
    GeneratorSwitch(const VariableProxy &switch_var,
                    list<OperatorProxy> &&immediate_operators,
                    const vector<GeneratorBase *> &&generator_for_value,
                    GeneratorBase *default_generator);
    virtual void dump(string indent) const;
    virtual void generate_cpp_input(ofstream &outfile) const;
};

class GeneratorLeaf : public GeneratorBase {
    list<OperatorProxy> applicable_operators;
public:
    GeneratorLeaf(list<OperatorProxy> &&applicable_operators);
    virtual void dump(string indent) const;
    virtual void generate_cpp_input(ofstream &outfile) const;
};

class GeneratorEmpty : public GeneratorBase {
public:
    virtual void dump(string indent) const;
    virtual void generate_cpp_input(ofstream &outfile) const;
};

GeneratorSwitch::GeneratorSwitch(
    const VariableProxy &switch_var, list<OperatorProxy> &&immediate_operators,
    const vector<GeneratorBase *> &&generator_for_value,
    GeneratorBase *default_generator)
    : switch_var(switch_var),
      immediate_operators(move(immediate_operators)),
      generator_for_value(move(generator_for_value)),
      default_generator(default_generator) {
}

GeneratorSwitch::~GeneratorSwitch() {
    for (GeneratorBase * generator : generator_for_value)
        delete generator;
    delete default_generator;
}

void GeneratorSwitch::dump(string indent) const {
    cout << indent << "switch on " << switch_var.get_name() << endl;
    cout << indent << "immediately:" << endl;
    for (const OperatorProxy &op : immediate_operators)
        cout << indent << op.get_name() << endl;
    for (int val = 0; val < switch_var.get_domain_size(); ++val) {
        cout << indent << "case "
             << switch_var.get_fact(val).get_name() << ":" << endl;
        generator_for_value[val]->dump(indent + "  ");
    }
    cout << indent << "always:" << endl;
    default_generator->dump(indent + "  ");
}

void GeneratorSwitch::generate_cpp_input(ofstream &outfile) const {
// TODO level not supported in task interface.
//    int level = switch_var->get_level();
//    assert(level != -1);
//    outfile << "switch " << level << endl;
    outfile << "check " << immediate_operators.size() << endl;
    for (const OperatorProxy &op : immediate_operators)
        outfile << op.get_id() << endl;
    for (int val = 0; val < switch_var.get_domain_size(); ++val) {
        generator_for_value[val]->generate_cpp_input(outfile);
    }
    default_generator->generate_cpp_input(outfile);
}

GeneratorLeaf::GeneratorLeaf(list<OperatorProxy> &&applicable_operators)
    : applicable_operators(move(applicable_operators)) {
}

void GeneratorLeaf::dump(string indent) const {
    for (const OperatorProxy &op : applicable_operators)
        cout << indent << op.get_name() << endl;
}

void GeneratorLeaf::generate_cpp_input(ofstream &outfile) const {
    outfile << "check " << applicable_operators.size() << endl;
    for (const OperatorProxy &op : applicable_operators)
        outfile << op.get_id() << endl;
}

void GeneratorEmpty::dump(string indent) const {
    cout << indent << "<empty>" << endl;
}

void GeneratorEmpty::generate_cpp_input(ofstream &outfile) const {
    outfile << "check 0" << endl;
}

SuccessorGenerator::SuccessorGenerator(shared_ptr<AbstractTask> task)
    : task(task),
      task_proxy(*task) {
    OperatorsProxy operators = task_proxy.get_operators();
    // We need the iterators to conditions to be stable:
    conditions.reserve(operators.size());
    list<OperatorProxy> all_operators;
    for (OperatorProxy op : operators) {
        Condition cond;
        for (FactProxy pre : op.get_preconditions()) {
            cond.push_back(make_pair(pre.get_variable(), pre.get_value()));
        }
        // Conditions must be ordered by variable id.
        sort(cond.begin(), cond.end());
        all_operators.push_back(op);
        conditions.push_back(cond);
        next_condition_by_op.push_back(conditions.back().begin());
    }

    root = construct_recursive(0, all_operators);
}

SuccessorGenerator::SuccessorGenerator() {
    root = 0;
}

SuccessorGenerator::~SuccessorGenerator() {
    delete root;
}

GeneratorBase *SuccessorGenerator::construct_recursive(
    int switch_var_id, list<OperatorProxy> &operator_queue) {
    if (operator_queue.empty())
        return new GeneratorEmpty;

    VariablesProxy variables = task_proxy.get_variables();

    while (true) {
        // Test if no further switch is necessary (or possible).
        if (switch_var_id == variables.size())
            return new GeneratorLeaf(move(operator_queue));

        VariableProxy switch_var = variables[switch_var_id];
        int number_of_children = switch_var.get_domain_size();

        vector<list<OperatorProxy> > operators_for_val(number_of_children);
        list<OperatorProxy> default_operators;
        list<OperatorProxy> applicable_operators;

        bool all_ops_are_immediate = true;
        bool var_is_interesting = false;

        while (!operator_queue.empty()) {
            OperatorProxy op = operator_queue.front();
            operator_queue.pop_front();
            int op_id = op.get_id();
            assert(op_id >= 0 && op_id < next_condition_by_op.size());
            Condition::const_iterator &cond_iter = next_condition_by_op[op_id];
            assert(cond_iter - conditions[op_id].begin() >= 0);
            assert(cond_iter - conditions[op_id].begin()
                   <= conditions[op_id].size());
            if (cond_iter == conditions[op_id].end()) {
                var_is_interesting = true;
                applicable_operators.push_back(op);
            } else {
                all_ops_are_immediate = false;
                const VariableProxy &var = cond_iter->first;
                int val = cond_iter->second;
                if (var == switch_var) {
                    var_is_interesting = true;
                    while (cond_iter != conditions[op_id].end() &&
                           cond_iter->first == switch_var) {
                        ++cond_iter;
                    }
                    operators_for_val[val].push_back(op);
                } else {
                    default_operators.push_back(op);
                }
            }
        }

        if (all_ops_are_immediate) {
            return new GeneratorLeaf(move(applicable_operators));
        } else if (var_is_interesting) {
            vector<GeneratorBase *> generator_for_val;
            for (list<OperatorProxy> ops : operators_for_val) {
                generator_for_val.push_back(
                    construct_recursive(switch_var_id + 1, ops));
            }
            GeneratorBase *default_generator = construct_recursive(
                switch_var_id + 1, default_operators);
            return new GeneratorSwitch(switch_var,
                                       move(applicable_operators),
                                       move(generator_for_val),
                                       default_generator);
        } else {
            // this switch var can be left out because no operator depends on it
            ++switch_var_id;
            operator_queue(move(default_operators));
        }
    }
}

/*
  TODO: this is a dummy implementation that will be replaced with code from the
  preprocessor in issue547. For now, we just loop through operators every time.
*/
void SuccessorGenerator::generate_applicable_ops(
    const State &state, std::vector<OperatorProxy> &applicable_ops) {
    for (OperatorProxy op : task_proxy.get_operators()) {
        if (is_applicable(op, state)) {
            applicable_ops.push_back(op);
        }
    }
}

void SuccessorGenerator::dump() const {
    cout << "Successor Generator:" << endl;
    root->dump("  ");
}
void SuccessorGenerator::generate_cpp_input(ofstream &outfile) const {
    root->generate_cpp_input(outfile);
}
