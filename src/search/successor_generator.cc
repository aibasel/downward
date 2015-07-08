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
    list<int> immediate_operators_ids;
    vector<GeneratorBase *> generator_for_value;
    GeneratorBase *default_generator;
public:
    ~GeneratorSwitch();
    GeneratorSwitch(const VariableProxy &switch_var,
                    list<int> &immediate_operators_ids,
                    const vector<GeneratorBase *> &generator_for_value,
                    GeneratorBase *default_generator);
    virtual void dump(string indent) const;
    virtual void generate_cpp_input(ofstream &outfile) const;
};

class GeneratorLeaf : public GeneratorBase {
    list<int> applicable_operator_ids;
public:
    GeneratorLeaf(list<int> &applicable_operator_ids);
    virtual void dump(string indent) const;
    virtual void generate_cpp_input(ofstream &outfile) const;
};

class GeneratorEmpty : public GeneratorBase {
public:
    virtual void dump(string indent) const;
    virtual void generate_cpp_input(ofstream &outfile) const;
};

GeneratorSwitch::GeneratorSwitch(
    const VariableProxy &switch_var, list<int> &immediate_operators_ids,
    const vector<GeneratorBase *> &generator_for_value,
    GeneratorBase *default_generator)
    : switch_var(switch_var),
      generator_for_value(generator_for_value),
      default_generator(default_generator) {
    immediate_operators_ids.swap(operators);
}

GeneratorSwitch::~GeneratorSwitch() {
    for (int i = 0; i < generator_for_value.size(); i++)
        delete generator_for_value[i];
    delete default_generator;
}

void GeneratorSwitch::dump(string indent) const {
    cout << indent << "switch on " << switch_var.get_name() << endl;
    cout << indent << "immediately:" << endl;
    for (list<int>::const_iterator op_iter = immediate_operators_ids.begin();
         op_iter != immediate_operators_ids.end(); ++op_iter)
        cout << indent << *op_iter << endl;
    for (int i = 0; i < switch_var.get_domain_size(); i++) {
        cout << indent << "case " << i << ":" << endl;
        generator_for_value[i]->dump(indent + "  ");
    }
    cout << indent << "always:" << endl;
    default_generator->dump(indent + "  ");
}

void GeneratorSwitch::generate_cpp_input(ofstream &outfile) const {
    int level = switch_var->get_level();
    assert(level != -1);
    outfile << "switch " << level << endl;
    outfile << "check " << immediate_operators_ids.size() << endl;
    for (list<int>::const_iterator op_iter = immediate_operators_ids.begin();
         op_iter != immediate_operators_ids.end(); ++op_iter)
        outfile << *op_iter << endl;
    for (int i = 0; i < switch_var.get_domain_size(); i++) {
        //cout << "case "<<switch_var->get_name()<<" (Level " <<switch_var->get_level() <<
        //  ") has value " << i << ":" << endl;
        generator_for_value[i]->generate_cpp_input(outfile);
    }
    //cout << "always:" << endl;
    default_generator->generate_cpp_input(outfile);
}

GeneratorLeaf::GeneratorLeaf(list<int> &applicable_operator_ids) {
    applicable_operator_ids.swap(applicable_operator_ids);
}

void GeneratorLeaf::dump(string indent) const {
    for (list<int>::const_iterator op_iter = applicable_operator_ids.begin();
         op_iter != applicable_operator_ids.end(); ++op_iter)
        cout << indent << *op_iter << endl;
}

void GeneratorLeaf::generate_cpp_input(ofstream &outfile) const {
    outfile << "check " << applicable_operator_ids.size() << endl;
    for (list<int>::const_iterator op_iter = applicable_operator_ids.begin();
         op_iter != applicable_operator_ids.end(); ++op_iter)
        outfile << *op_iter << endl;
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
    list<int> all_operator_ids;
    for (OperatorProxy op : operators) {
        Condition cond;
        for (FactProxy pre : op.get_preconditions()) {
            cond.push_back(make_pair(pre.get_variable(), pre.get_value()));
        }
        // Conditions must be ordered by variable id.
        sort(cond.begin(), cond.end());
        all_operator_ids.push_back(op.get_id());
        conditions.push_back(cond);
        next_condition_by_op.push_back(conditions.back().begin());
    }

    root = construct_recursive(0, all_operator_ids);
}

SuccessorGenerator::SuccessorGenerator() {
    root = 0;
}

SuccessorGenerator::~SuccessorGenerator() {
    delete root;
}

GeneratorBase *SuccessorGenerator::construct_recursive(int switch_var_id,
                                                       list<int> &operator_ids) {
    if (operator_ids.empty())
        return new GeneratorEmpty;

    VariablesProxy variables = task_proxy.get_variables();

    while (true) {
        // Test if no further switch is necessary (or possible).
        if (switch_var_id == variables.size())
            return new GeneratorLeaf(operator_ids);

        VariableProxy switch_var = variables[switch_var_id];
        int number_of_children = switch_var->get_range();

        vector<list<int> > ops_for_val_ids(number_of_children);
        list<int> default_op_ids;
        list<int> applicable_op_ids;

        bool all_ops_are_immediate = true;
        bool var_is_interesting = false;

        while (!operator_ids.empty()) {
            int op_id = operator_ids.front();
            operator_ids.pop_front();
            assert(op_id >= 0 && op_id < next_condition_by_op.size());
            Condition::const_iterator &cond_iter = next_condition_by_op[op_id];
            assert(cond_iter - conditions[op_id].begin() >= 0);
            assert(cond_iter - conditions[op_id].begin() <= conditions[op_id].size());
            if (cond_iter == conditions[op_id].end()) {
                var_is_interesting = true;
                applicable_op_ids.push_back(op_id);
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
                    ops_for_val_ids[val].push_back(op_id);
                } else {
                    default_op_ids.push_back(op_id);
                }
            }
        }

        if (all_ops_are_immediate) {
            return new GeneratorLeaf(applicable_op_ids);
        } else if (var_is_interesting) {
            vector<GeneratorBase *> generator_for_val;
            for (int j = 0; j < number_of_children; j++) {
                generator_for_val.push_back(construct_recursive(
                    switch_var_id + 1, ops_for_val_ids[j]));
            }
            GeneratorBase *default_sg = construct_recursive(
                switch_var_id + 1, default_op_ids);
            return new GeneratorSwitch(switch_var, applicable_op_ids,
                                       generator_for_val, default_sg);
        } else {
            // this switch var can be left out because no operator depends on it
            ++switch_var_id;
            default_op_ids.swap(operator_ids);
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
